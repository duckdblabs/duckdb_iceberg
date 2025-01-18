#include "catalog_api.hpp"
#include "storage/ic_catalog.hpp"
#include "yyjson.hpp"
#include <curl/curl.h>
#include <sys/stat.h>

#include <iostream>

namespace duckdb {

//! We use a global here to store the path that is selected on the IBAPI::InitializeCurl call
static string SELECTED_CURL_CERT_PATH = "";

static size_t GetRequestWriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	((std::string *)userp)->append((char *)contents, size * nmemb);
	return size * nmemb;
}

// we statically compile in libcurl, which means the cert file location of the build machine is the
// place curl will look. But not every distro has this file in the same location, so we search a
// number of common locations and use the first one we find.
static string certFileLocations[] = {
        // Arch, Debian-based, Gentoo
        "/etc/ssl/certs/ca-certificates.crt",
        // RedHat 7 based
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
        // Redhat 6 based
        "/etc/pki/tls/certs/ca-bundle.crt",
        // OpenSUSE
        "/etc/ssl/ca-bundle.pem",
        // Alpine
        "/etc/ssl/cert.pem"
};

// Look through the the above locations and if one of the files exists, set that as the location curl should use.
static bool SelectCurlCertPath() {
	for (string& caFile : certFileLocations) {
		struct stat buf;
		if (stat(caFile.c_str(), &buf) == 0) {
			SELECTED_CURL_CERT_PATH = caFile;
		}
	}
	return false;
}

static bool SetCurlCAFileInfo(CURL* curl) {
	if (!SELECTED_CURL_CERT_PATH.empty()) {
		curl_easy_setopt(curl, CURLOPT_CAINFO, SELECTED_CURL_CERT_PATH.c_str());
        return true;
	}
    return false;
}

// Note: every curl object we use should set this, because without it some linux distro's may not find the CA certificate.
static void InitializeCurlObject(CURL * curl, const string &token) {
  	if (!token.empty()) {
		curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, token.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
	}
    SetCurlCAFileInfo(curl);
}

static string GetRequest(const string &url, const string &token = "", curl_slist *extra_headers = NULL) {
	CURL *curl;
	CURLcode res;
	string readBuffer;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, GetRequestWriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		if(extra_headers) {
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, extra_headers);
		}
		InitializeCurlObject(curl, token);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if (res != CURLcode::CURLE_OK) {
			string error = curl_easy_strerror(res);
			throw IOException("Curl Request to '%s' failed with error: '%s'", url, error);
		}
		return readBuffer;
	}
	throw InternalException("Failed to initialize curl");
}

static string PostRequest(const string &url, const string &post_data, curl_slist *extra_headers = NULL) {
    string readBuffer;
    CURL *curl = curl_easy_init();
    if (!curl) {
		throw InternalException("Failed to initialize curl");
	}

	// Set the URL
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	
	// Set POST method
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	
	// Set POST data
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
	
	// Set up response handling
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, GetRequestWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	// Create default headers for content type
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
	
	// Append any extra headers
	if (extra_headers) {
		struct curl_slist *temp = extra_headers;
		while (temp) {
			headers = curl_slist_append(headers, temp->data);
			temp = temp->next;
		}
	}
	
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	InitializeCurlObject(curl, "");
	
	// Perform the request
	CURLcode res = curl_easy_perform(curl);
	
	// Clean up
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLcode::CURLE_OK) {
		string error = curl_easy_strerror(res);
		throw IOException("Curl Request to '%s' failed with error: '%s'", url, error);
	}
	return readBuffer;
    
}

static duckdb_yyjson::yyjson_val *GetTableMetadata(const string &internal, const string &schema, const string &table, IBCredentials credentials) {
	struct curl_slist *extra_headers = NULL;
	extra_headers = curl_slist_append(extra_headers, "X-Iceberg-Access-Delegation: vended-credentials");
	auto api_result = GetRequest(
		credentials.endpoint + "/v1/" + internal + "/namespaces/" + schema + "/tables/" + table, 
		credentials.token,
		extra_headers);
	duckdb_yyjson::yyjson_doc *doc = duckdb_yyjson::yyjson_read(api_result.c_str(), api_result.size(), 0);
	return yyjson_doc_get_root(doc);
}

template <class TYPE, uint8_t TYPE_NUM, TYPE (*get_function)(duckdb_yyjson::yyjson_val *obj)>
static TYPE TemplatedTryGetYYJson(duckdb_yyjson::yyjson_val *obj, const string &field, TYPE default_val,
                                  bool fail_on_missing = true) {
	auto val = yyjson_obj_get(obj, field.c_str());
	if (val && yyjson_get_type(val) == TYPE_NUM) {
		return get_function(val);
	} else if (!fail_on_missing) {
		return default_val;
	}
	throw IOException("Invalid field found while parsing field: " + field);
}

static uint64_t TryGetNumFromObject(duckdb_yyjson::yyjson_val *obj, const string &field, bool fail_on_missing = true,
                                    uint64_t default_val = 0) {
	return TemplatedTryGetYYJson<uint64_t, YYJSON_TYPE_NUM, duckdb_yyjson::yyjson_get_uint>(obj, field, default_val,
	                                                                                        fail_on_missing);
}
static bool TryGetBoolFromObject(duckdb_yyjson::yyjson_val *obj, const string &field, bool fail_on_missing = false,
                                 bool default_val = false) {
	return TemplatedTryGetYYJson<bool, YYJSON_TYPE_BOOL, duckdb_yyjson::yyjson_get_bool>(obj, field, default_val,
	                                                                                     fail_on_missing);
}
static string TryGetStrFromObject(duckdb_yyjson::yyjson_val *obj, const string &field, bool fail_on_missing = true,
                                  const char *default_val = "") {
	return TemplatedTryGetYYJson<const char *, YYJSON_TYPE_STR, duckdb_yyjson::yyjson_get_str>(obj, field, default_val,
	                                                                                           fail_on_missing);
}

void IBAPI::InitializeCurl() {
	SelectCurlCertPath();
}

vector<string> IBAPI::GetCatalogs(const string &catalog, IBCredentials credentials) {
	throw NotImplementedException("IBAPI::GetCatalogs");
}

static IBAPIColumnDefinition ParseColumnDefinition(duckdb_yyjson::yyjson_val *column_def) {
	IBAPIColumnDefinition result;

	result.name = TryGetStrFromObject(column_def, "name");
	result.type_text = TryGetStrFromObject(column_def, "type");
	result.precision = -1; //TODO: TryGetNumFromObject(column_def, "type_precision");
	result.scale = -1; //TODO: TryGetNumFromObject(column_def, "type_scale");
	result.position = TryGetNumFromObject(column_def, "id") - 1;

	return result;
}

IBAPITableCredentials IBAPI::GetTableCredentials(const string &internal, const string &schema, const string &table, IBCredentials credentials) {
	IBAPITableCredentials result;

	duckdb_yyjson::yyjson_val *root = GetTableMetadata(internal, schema, table, credentials);
	auto *aws_temp_credentials = yyjson_obj_get(root, "config");

	if (aws_temp_credentials) {
		result.key_id = TryGetStrFromObject(aws_temp_credentials, "s3.access-key-id");
		result.secret = TryGetStrFromObject(aws_temp_credentials, "s3.secret-access-key");
		result.session_token = TryGetStrFromObject(aws_temp_credentials, "s3.session-token");
	}

	return result;
}

string IBAPI::GetToken(string id, string secret, string endpoint) {
	string post_data = "grant_type=client_credentials&client_id=" + id + "&client_secret=" + secret + "&scope=PRINCIPAL_ROLE:ALL";
	string api_result = PostRequest(endpoint + "/v1/oauth/tokens", post_data);

	// Read JSON and get root
	auto *doc = duckdb_yyjson::yyjson_read(api_result.c_str(), api_result.size(), 0);
	auto *root = yyjson_doc_get_root(doc);

	auto *error = yyjson_obj_get(root, "error");
	if (error != NULL) {
		string err_msg = TryGetStrFromObject(error, "message");
		throw std::runtime_error(err_msg);
	}

	return TryGetStrFromObject(root, "access_token");
}

vector<IBAPITable> IBAPI::GetTables(const string &catalog, const string &internal, const string &schema, IBCredentials credentials) {
	vector<IBAPITable> result;

	auto api_result = GetRequest(credentials.endpoint + "/v1/" + internal + "/namespaces/" + schema + "/tables", credentials.token);

	// Read JSON and get root
	auto *doc = duckdb_yyjson::yyjson_read(api_result.c_str(), api_result.size(), 0);
	auto *root = yyjson_doc_get_root(doc);

	// Get root["hits"], iterate over the array
	auto *tables = yyjson_obj_get(root, "identifiers");
	size_t idx, max;
	duckdb_yyjson::yyjson_val *table;
	yyjson_arr_foreach(tables, idx, max, table) {
		IBAPITable table_result;
		table_result.catalog_name = catalog;
		table_result.schema_name = schema;
		table_result.name = TryGetStrFromObject(table, "name");

		auto *metadata_root = GetTableMetadata(internal, schema, table_result.name, credentials);
		table_result.data_source_format = "ICEBERG";
		table_result.storage_location = TryGetStrFromObject(metadata_root, "metadata-location");
		auto *metadata = yyjson_obj_get(metadata_root, "metadata");
		table_result.table_id = TryGetStrFromObject(metadata, "table-uuid");

		uint64_t current_schema_id = TryGetNumFromObject(metadata, "current-schema-id");
		auto *schemas = yyjson_obj_get(metadata, "schemas");
		duckdb_yyjson::yyjson_val *schema;
		size_t schema_idx, schema_max;
		bool found = false;
		yyjson_arr_foreach(schemas, schema_idx, schema_max, schema) {
			uint64_t schema_id = TryGetNumFromObject(schema, "schema-id");
			if (schema_id == current_schema_id) {
				found = true;
				auto *columns = yyjson_obj_get(schema, "fields");
				duckdb_yyjson::yyjson_val *col;
				size_t col_idx, col_max;
				yyjson_arr_foreach(columns, col_idx, col_max, col) {
					auto column_definition = ParseColumnDefinition(col);
					table_result.columns.push_back(column_definition);
				}
				result.push_back(table_result);

				// This could take a while so display found tables to indicate progress
				// TODO: Is there a way to show progress some other way?
				std::cout << table_result.schema_name << " | " << table_result.name << std::endl;
			}
		}

		if (!found) {
			throw InternalException("Current schema not found");
		}
	}

	return result;
}

vector<IBAPISchema> IBAPI::GetSchemas(const string &catalog, const string &internal, IBCredentials credentials) {
	vector<IBAPISchema> result;

	auto api_result =
	    GetRequest(credentials.endpoint + "/v1/" + internal + "/namespaces", credentials.token);

	// Read JSON and get root
	duckdb_yyjson::yyjson_doc *doc = duckdb_yyjson::yyjson_read(api_result.c_str(), api_result.size(), 0);
	duckdb_yyjson::yyjson_val *root = yyjson_doc_get_root(doc);

	auto *error = yyjson_obj_get(root, "error");
	if (error != NULL) {
		string err_msg = TryGetStrFromObject(error, "message");
		throw std::runtime_error(err_msg);
	}

	// Get root["hits"], iterate over the array
	auto *schemas = yyjson_obj_get(root, "namespaces");
	size_t idx, max;
	duckdb_yyjson::yyjson_val *schema;
	yyjson_arr_foreach(schemas, idx, max, schema) {
		IBAPISchema schema_result;

		schema_result.catalog_name = catalog;
		duckdb_yyjson::yyjson_val *value = yyjson_arr_get(schema, 0);
		schema_result.schema_name = yyjson_get_str(value);
		result.push_back(schema_result);
	}

	return result;
}

} // namespace duckdb

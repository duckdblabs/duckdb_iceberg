
#pragma once

#include "duckdb/common/types.hpp"

namespace duckdb {
struct IBCredentials;

struct IBAPIColumnDefinition {
	string name;
	string type_text;
	idx_t precision;
	idx_t scale;
	idx_t position;
};

struct IBAPITable {
	string table_id;

	string name;
	string catalog_name;
	string schema_name;
	string table_type;
	string data_source_format;

	string storage_location;

	vector<IBAPIColumnDefinition> columns;
};

struct IBAPISchema {
	string schema_name;
	string catalog_name;
};

struct IBAPITableCredentials {
	string key_id;
	string secret;
	string session_token;
};

class IBAPI {
public:
  	//! WARNING: not thread-safe. To be called once on extension initialization
  	static void InitializeCurl();

	static IBAPITableCredentials GetTableCredentials(const string &internal, const string &schema, const string &table, IBCredentials credentials);
	static vector<string> GetCatalogs(const string &catalog, IBCredentials credentials);
	static vector<IBAPITable> GetTables(const string &catalog, const string &internal, const string &schema, IBCredentials credentials);
	static vector<IBAPISchema> GetSchemas(const string &catalog, const string &internal, IBCredentials credentials);
	static vector<IBAPITable> GetTablesInSchema(const string &catalog, const string &schema, IBCredentials credentials);
	static string GetToken(string id, string secret, string endpoint);
};
} // namespace duckdb

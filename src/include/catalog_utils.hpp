
#pragma once

#include "duckdb.hpp"
#include "catalog_api.hpp"

namespace duckdb {
class IBSchemaEntry;
class IBTransaction;

enum class IBTypeAnnotation { STANDARD, CAST_TO_VARCHAR, NUMERIC_AS_DOUBLE, CTID, JSONB, FIXED_LENGTH_CHAR };

struct IBType {
	idx_t oid = 0;
	IBTypeAnnotation info = IBTypeAnnotation::STANDARD;
	vector<IBType> children;
};

class IBUtils {
public:
	static LogicalType ToUCType(const LogicalType &input);
	static LogicalType TypeToLogicalType(ClientContext &context, const string &columnDefinition);
	static string TypeToString(const LogicalType &input);
};

} // namespace duckdb

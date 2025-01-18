#include "storage/ic_schema_entry.hpp"
#include "storage/ic_table_entry.hpp"
#include "storage/ic_transaction.hpp"
#include "duckdb/parser/parsed_data/create_view_info.hpp"
#include "duckdb/parser/parsed_data/create_index_info.hpp"
#include "duckdb/planner/parsed_data/bound_create_table_info.hpp"
#include "duckdb/parser/parsed_data/drop_info.hpp"
#include "duckdb/parser/constraints/list.hpp"
#include "duckdb/common/unordered_set.hpp"
#include "duckdb/parser/parsed_data/alter_info.hpp"
#include "duckdb/parser/parsed_data/alter_table_info.hpp"
#include "duckdb/parser/parsed_expression_iterator.hpp"

namespace duckdb {

IBSchemaEntry::IBSchemaEntry(Catalog &catalog, CreateSchemaInfo &info)
    : SchemaCatalogEntry(catalog, info), tables(*this) {
}

IBSchemaEntry::~IBSchemaEntry() {
}

IBTransaction &GetUCTransaction(CatalogTransaction transaction) {
	if (!transaction.transaction) {
		throw InternalException("No transaction!?");
	}
	return transaction.transaction->Cast<IBTransaction>();
}

optional_ptr<CatalogEntry> IBSchemaEntry::CreateTable(CatalogTransaction transaction, BoundCreateTableInfo &info) {
	auto &base_info = info.Base();
	auto table_name = base_info.table;
	if (base_info.on_conflict == OnCreateConflict::REPLACE_ON_CONFLICT) {
		throw NotImplementedException("REPLACE ON CONFLICT in CreateTable");
	}
	return tables.CreateTable(transaction.GetContext(), info);
}

optional_ptr<CatalogEntry> IBSchemaEntry::CreateFunction(CatalogTransaction transaction, CreateFunctionInfo &info) {
	throw BinderException("PC databases do not support creating functions");
}

void IBUnqualifyColumnRef(ParsedExpression &expr) {
	if (expr.type == ExpressionType::COLUMN_REF) {
		auto &colref = expr.Cast<ColumnRefExpression>();
		auto name = std::move(colref.column_names.back());
		colref.column_names = {std::move(name)};
		return;
	}
	ParsedExpressionIterator::EnumerateChildren(expr, IBUnqualifyColumnRef);
}

optional_ptr<CatalogEntry> IBSchemaEntry::CreateIndex(CatalogTransaction transaction, CreateIndexInfo &info,
                                                      TableCatalogEntry &table) {
	throw NotImplementedException("CreateIndex");
}

string GetUCCreateView(CreateViewInfo &info) {
	throw NotImplementedException("GetCreateView");
}

optional_ptr<CatalogEntry> IBSchemaEntry::CreateView(CatalogTransaction transaction, CreateViewInfo &info) {
	if (info.sql.empty()) {
		throw BinderException("Cannot create view in PC that originated from an "
		                      "empty SQL statement");
	}
	if (info.on_conflict == OnCreateConflict::REPLACE_ON_CONFLICT ||
	    info.on_conflict == OnCreateConflict::IGNORE_ON_CONFLICT) {
		auto current_entry = GetEntry(transaction, CatalogType::VIEW_ENTRY, info.view_name);
		if (current_entry) {
			if (info.on_conflict == OnCreateConflict::IGNORE_ON_CONFLICT) {
				return current_entry;
			}
			throw NotImplementedException("REPLACE ON CONFLICT in CreateView");
		}
	}
	auto &ic_transaction = GetUCTransaction(transaction);
	//	ic_transaction.Query(GetUCCreateView(info));
	return tables.RefreshTable(transaction.GetContext(), info.view_name);
}

optional_ptr<CatalogEntry> IBSchemaEntry::CreateType(CatalogTransaction transaction, CreateTypeInfo &info) {
	throw BinderException("PC databases do not support creating types");
}

optional_ptr<CatalogEntry> IBSchemaEntry::CreateSequence(CatalogTransaction transaction, CreateSequenceInfo &info) {
	throw BinderException("PC databases do not support creating sequences");
}

optional_ptr<CatalogEntry> IBSchemaEntry::CreateTableFunction(CatalogTransaction transaction,
                                                              CreateTableFunctionInfo &info) {
	throw BinderException("PC databases do not support creating table functions");
}

optional_ptr<CatalogEntry> IBSchemaEntry::CreateCopyFunction(CatalogTransaction transaction,
                                                             CreateCopyFunctionInfo &info) {
	throw BinderException("PC databases do not support creating copy functions");
}

optional_ptr<CatalogEntry> IBSchemaEntry::CreatePragmaFunction(CatalogTransaction transaction,
                                                               CreatePragmaFunctionInfo &info) {
	throw BinderException("PC databases do not support creating pragma functions");
}

optional_ptr<CatalogEntry> IBSchemaEntry::CreateCollation(CatalogTransaction transaction, CreateCollationInfo &info) {
	throw BinderException("PC databases do not support creating collations");
}

void IBSchemaEntry::Alter(CatalogTransaction transaction, AlterInfo &info) {
	if (info.type != AlterType::ALTER_TABLE) {
		throw BinderException("Only altering tables is supported for now");
	}
	auto &alter = info.Cast<AlterTableInfo>();
	tables.AlterTable(transaction.GetContext(), alter);
}

bool CatalogTypeIsSupported(CatalogType type) {
	switch (type) {
	case CatalogType::INDEX_ENTRY:
	case CatalogType::TABLE_ENTRY:
	case CatalogType::VIEW_ENTRY:
		return true;
	default:
		return false;
	}
}

void IBSchemaEntry::Scan(ClientContext &context, CatalogType type,
                         const std::function<void(CatalogEntry &)> &callback) {
	if (!CatalogTypeIsSupported(type)) {
		return;
	}
	GetCatalogSet(type).Scan(context, callback);
}
void IBSchemaEntry::Scan(CatalogType type, const std::function<void(CatalogEntry &)> &callback) {
	throw NotImplementedException("Scan without context not supported");
}

void IBSchemaEntry::DropEntry(ClientContext &context, DropInfo &info) {
	GetCatalogSet(info.type).DropEntry(context, info);
}

optional_ptr<CatalogEntry> IBSchemaEntry::GetEntry(CatalogTransaction transaction, CatalogType type,
                                                   const string &name) {
	if (!CatalogTypeIsSupported(type)) {
		return nullptr;
	}
	return GetCatalogSet(type).GetEntry(transaction.GetContext(), name);
}

IBCatalogSet &IBSchemaEntry::GetCatalogSet(CatalogType type) {
	switch (type) {
	case CatalogType::TABLE_ENTRY:
	case CatalogType::VIEW_ENTRY:
		return tables;
	default:
		throw InternalException("Type not supported for GetCatalogSet");
	}
}

} // namespace duckdb

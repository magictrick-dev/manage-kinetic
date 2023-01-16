#ifndef MANAGE_KINETIC_MAPPING_TABLE_H
#define MANAGE_KINETIC_MAPPING_TABLE_H
#include <vector>
#include <string>
#include <mutex>
#include <memory>

#include <managekinetic/primitives.h>

class MappingTable;
class MappingRecord;
class MappingField;

class MappingField
{
	public:
		~MappingField() 	{};

		void insertValue(std::string value)
		{
			this->values.push_back(value);
		}

		std::string& operator[](ui64 index)
		{
			return this->values[index];
		}

		ui64 count()
		{
			return this->values.size();
		}

	protected:
		std::vector<std::string> values;
		MappingField(MappingTable* parentTable, MappingRecord* parentRecord) 
			: parentTable(parentTable), parentRecord(parentRecord) {};

	private:
		MappingTable* parentTable;
		MappingRecord* parentRecord;
		friend MappingTable;
		friend MappingRecord;
};

class MappingRecord
{
	public:
		~MappingRecord() 	{};

		MappingField& createField()
		{
			this->fields.push_back({this->parent, this});
			return this->fields.back();
		}

		MappingField& operator[](ui64 index)
		{
			return this->fields[index];
		}

		ui64 count()
		{
			return this->fields.size();
		}

	protected:
		std::vector<MappingField> fields;
		MappingRecord(MappingTable* parent) : parent(parent) {};

	private:
		MappingTable* parent;
		friend MappingTable;
};

class MappingTable
{
	public:
		MappingTable() 		{};
		~MappingTable() 	{};


		MappingRecord& createRecord()
		{
			this->records.push_back({this});
			return this->records.back();
		};


		MappingRecord& operator[](ui64 index)
		{
			return this->records[index];
		}


		ui64 count()
		{
			return this->records.size();
		}

	protected:
		std::vector<MappingRecord> records;

};

class MappingDatabase
{
	public:
		MappingDatabase() {};
		~MappingDatabase()
		{
			
		};



	protected:
		std::vector<MappingTable> tables;
};

// -----------------------------------------------------------------------------
// Refactor of MappingTable ==> tablemap
// -----------------------------------------------------------------------------

class tablemap;
class subtable;
class record;
struct field;

class field
{
	public:
		field(record* parentRecord);
		field(const field&);
		~field();

		std::string operator[](ui64 index);

		operator std::string() const { return this->get(); };
		field& operator=(const field&);

		std::string get() const;
		std::string get(std::string delim) const;
		std::string get(ui64 index) const;

		ui64 valueCount() const { return this->values.size(); }

	private:

		record* parentRecord;
		
		std::vector<char*> values;
		
		friend record;
};

/**
 * A record is simply a row within a subtable. A record can contain n-fields.
 */
class record
{
	public:
		~record();

		field& insertField(std::string);
		field& insertMultivalueField(std::vector<std::string>);

		ui64 index() const;

		field* operator[](ui64 index);

	protected:
		record(tablemap* parentTable, subtable* parentSubtable);

		std::vector<field> fields;

		tablemap* parentTable;
		subtable* parentSubtable;

		friend subtable;
		friend tablemap;
};

/**
 * Represents a partitioning of a tablemap. The underlying structure of this is
 * meant to be internal. At export time, each subtable is individually processed
 * such that they represent one Excel document. This behavior is governed by the
 * number of records per Excel document defined in parameters.
 */
class subtable
{
	public:
		subtable(tablemap* parent);
		~subtable();
	
		inline ui64 size() const { return this->records.size(); };

		record& create();

	protected:

		std::vector<record> records;

		tablemap* parent;
		friend tablemap;
		friend record;
};

struct column_map
{
	ui64 		column_index;
	i32 		multivalue_index;
	bool 		all_multivalues;
	std::string multivalue_delim;
	std::string export_alias = "";
};

/**
 * A tablemap represents the layout of data being imported into Manage Kinetic
 * and acts as the governing interface between the mapping tool and export facility.
 * This tablemap is partitioned and serves as the API front-end and serialization
 * structure. Use this interface to insert & fetch records as desired.
 */
class tablemap
{
	public:
		tablemap(ui32 maxRecordsPerSubtable);
		~tablemap();

		record& operator[](ui64 index);

		std::shared_ptr<subtable> createSubtable();

		ui64 count() const;

		inline ui64 columnsCount() const
			{ return this->columns.size(); };

		std::vector<std::string>& getColumnNames() { return this->columns; }
		std::vector<column_map>& getMappedColumns() { return this->mapped_columns; }

	protected:
		const ui32 maxRecordsPerSubtable;
		std::vector<std::shared_ptr<subtable>> subtables;

		std::vector<std::string> columns;
		std::vector<column_map> mapped_columns;

		void insertColumn(ui64 columnIndex);

	friend subtable;
	friend record;

};

#endif
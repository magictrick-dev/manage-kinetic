#ifndef MANAGE_KINETIC_MAPPING_TABLE_H
#define MANAGE_KINETIC_MAPPING_TABLE_H
#include <vector>
#include <string>
#include <mutex>
#include <memory>

#include <managekinetic/primitives.h>

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

		inline std::vector<record>& getRecords()
			{ return this->records; };

		record& create();

	protected:

		std::vector<record> records;

		tablemap* parent;
		friend tablemap;
		friend record;
};

struct __string_offset
{
	ui64 offset_location;
	ui64 string_size;
};

/**
 * When exporting a map, we need a format that can translate to binary. Unfortunately,
 * std::string does not support this feature and we will need to manually append
 * these out to a file.
 */
#pragma pack(push, 1)
#define COLUMN_MAP_VERSION_1 "MTV1"
struct __serialize_column_map_v1
{
	struct meta
	{
		char 	header[8] = COLUMN_MAP_VERSION_1;
		ui64 	next; // A value of 0 indicates no next, any positive indicates where
					  // The next values are located.
	} meta;

	
	ui64 	column_index;
	i32 	multivalue_index;
	bool 	all_multivalues;
	__string_offset 	multivalue_delim;
	__string_offset 	export_alias;


};
#pragma pack(pop)

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

		ui64 subtableCount() const
			{ return this->subtables.size(); }

		std::vector<std::shared_ptr<subtable>> getSubtables()
			{ return this->subtables; }

		inline ui64 columnsCount() const
			{ return this->columns.size(); };

		std::vector<std::string>& getColumnNames() { return this->columns; }
		std::vector<column_map>& getMappedColumns() { return this->mapped_columns; }

	protected:
		const ui32 maxRecordsPerSubtable;
		std::vector<std::shared_ptr<subtable>> subtables;

		std::mutex columns_name_lock;
		std::vector<std::string> columns;
		std::vector<column_map> mapped_columns;

		void insertColumn(ui64 columnIndex);

	friend subtable;
	friend record;

};

#endif
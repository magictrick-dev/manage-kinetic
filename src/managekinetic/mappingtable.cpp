/**
 * 
 * In order to improve efficiency of the program, data will need to be stored as
 * raw c-strings. In order to adequately allow for splitting, we need to store
 * everything into a table which allows for easy access as a global object, and
 * also as seperate units.
 * 
 * tablemap[n] => has access to all subtable splits.
 * 		0 					=> subtable[0].size() - 1
 * 		subtable[0].size() 	=> subtabletable[1].size() - 1
 * 		subtable[1].size() 	=> subtabletable[2].size() - 1
 * 		subtable[2].size() 	=> subtabletable[3].size() - 1
 * 		... etc.
 * 
 * tablemap.getsub(n)[n] => has access to just the values of the subtable.
 * 		0 	=> this.size() - 1
 * 
 * Each
 * 
 */

#include <managekinetic/primitives.h>
#include <managekinetic/mappingtable.h>
#include <managekinetic/application.h>

#include <sstream>
#include <string>

// ---------------------------------------------------------------------------------------------------------------------
// Mapping Table Definitions
// ---------------------------------------------------------------------------------------------------------------------

tablemap::
tablemap(ui32 maxRecordsPerSubtable)
	: maxRecordsPerSubtable(maxRecordsPerSubtable)
{

}

tablemap::
~tablemap()
{
	// The shared_ptr will handle the rest.
}

ui64 tablemap::
count() const
{

	ui64 total = 0;
	for (ui64 subtableIndex = 0; subtableIndex < this->subtables.size(); ++subtableIndex)
		total += this->subtables[subtableIndex]->size();
	
	return total;

}

#if 0
record& tablemap::
createRecord()
{

	// -------------------------------------------------------------------------
	// Step 1: Ensure that we have a valid table.
	// -------------------------------------------------------------------------
	
	if (this->subtables.size() <= 0)
		this->subtables.emplace_back(std::make_shared<subtable>(this));

	// -------------------------------------------------------------------------
	// Step 2: Fetch the first available subtable to accomodate the record.
	// -------------------------------------------------------------------------

	std::shared_ptr<subtable> currentSubtable = nullptr;

	for (ui64 subtableIndex = 0; subtableIndex < this->subtables.size(); ++subtableIndex)
		if (this->subtables[subtableIndex]->size() < this->maxRecordsPerSubtable)
		{
			currentSubtable = this->subtables[subtableIndex];
			break;
		}

	if (currentSubtable == nullptr)
	{
		this->subtables.emplace_back(std::make_shared<subtable>(this));
		currentSubtable = this->subtables.back();
	}

	// -------------------------------------------------------------------------
	// Step 3: Insert the record and then return it.
	// -------------------------------------------------------------------------

	return currentSubtable->create();

}

#endif

void tablemap::
insertColumn(ui64 columnIndex)
{
	std::stringstream columnName = {};
	columnName << "Column F" << columnIndex;
	this->columns.push_back(columnName.str());
}

std::shared_ptr<subtable> tablemap::
createSubtable()
{
	this->subtables.push_back(std::make_shared<subtable>(this));
	return this->subtables.back();
}

record& tablemap::
operator[](ui64 index)
{

	// Note that the search *assumes* that the index provided is valid and may
	// very well self-destruct should it not exist.

	ui64 searchIndex = index;
	
	ui64 subtableIndex = 0;
	std::shared_ptr<subtable> currentSubtable = this->subtables[subtableIndex];
	while (currentSubtable != nullptr)
	{

		// Decrement the search index if it isn't in bounds of the subtable.
		if (searchIndex >= currentSubtable->size())
		{
			searchIndex -= currentSubtable->size();
			currentSubtable = this->subtables[++subtableIndex];
		}
		else
		{
			break; // We know what table and index, break from loop.
		}

	}

	return currentSubtable->records[searchIndex];

}


// ---------------------------------------------------------------------------------------------------------------------
// Subtable Definitions
// ---------------------------------------------------------------------------------------------------------------------

subtable::
subtable(tablemap* parent)
	: parent(parent)
{ }

subtable::
~subtable()
{ }

record& subtable::
create()
{
	this->records.push_back({this->parent, this});
	record& currentRecord = this->records.back();
	return currentRecord;
}

// ---------------------------------------------------------------------------------------------------------------------
// Record Definitions
// ---------------------------------------------------------------------------------------------------------------------

record::
~record()
{ }

record::
record(tablemap* parentTable, subtable* parentSubtable)
	: parentTable(parentTable), parentSubtable(parentSubtable)
{ }

ui64 record::
index() const
{

	// Note that the index of a record may change over time, as some records may be
	// deleted or inserted at a position before. Therefore, we should always find
	// and return the index given this fact.

	// Initialize the index as 0.
	ui32 index = 0;

	// Go through all the tables up-to, not including the current subtable which
	// this record resides in.
	std::shared_ptr<subtable> currentSubtable = this->parentTable->subtables[0];
	while (currentSubtable.get() != this->parentSubtable)
		index += currentSubtable->size();

	// Now attempt to find the record within its own subtable.
	for (ui64 subIndex = 0; subIndex < this->parentSubtable->size(); ++subIndex)
		if (&this->parentSubtable->records[subIndex] == this)
		{
			index += subIndex;
			break;
		}
	
	// Once found, we know the calculated index and we can return it.
	return index;

}

field* record::
operator[](ui64 index)
{

	if (index >= this->fields.size())
		return nullptr;

	return &this->fields[index];

}

field& record::
insertField(std::string value)
{

	this->fields.push_back(field(this));
	field& currentField = this->fields.back();

	char* sbuffer = new char[value.length()+1];
	strcpy(sbuffer, value.c_str());
	currentField.values.emplace_back(sbuffer);

	if (this->fields.size() > this->parentTable->columns.size())
	{
		this->parentTable->columns_name_lock.lock();
		this->parentTable->insertColumn(this->fields.size() - 1);
		this->parentTable->columns_name_lock.unlock();
	}

	return currentField;

}

field& record::
insertMultivalueField(std::vector<std::string> values)
{

	this->fields.push_back(field(this));
	field& currentField = this->fields.back();

	for (ui64 vindex = 0; vindex < values.size(); ++vindex)
	{
		char* sbuffer = new char[ (values[vindex].length()+1) ];
		strcpy(sbuffer, values[vindex].c_str());
		currentField.values.push_back(sbuffer);
	}

	if (this->fields.size() > this->parentTable->columns.size())
	{
		this->parentTable->columns_name_lock.lock();
		this->parentTable->insertColumn(this->fields.size() - 1);
		this->parentTable->columns_name_lock.unlock();
	}

	return currentField;

}

// ---------------------------------------------------------------------------------------------------------------------
// Field Implementation
// ---------------------------------------------------------------------------------------------------------------------

field::
field(record* parentRecord)
	: parentRecord(parentRecord)
{ }

field::
field(const field& f)
{

	std::vector<std::string> mv = {};
	for (ui64 sindex = 0; sindex < f.values.size(); ++sindex)
	{
		std::string s = f.values[sindex];
		mv.push_back(s);
	}

	for (std::string s : mv)
	{
		char* sbuffer = new char[s.length() + 1];
		strcpy(sbuffer, s.c_str());
		this->values.push_back(sbuffer);
	}

}

field& field::
operator=(const field& f)
{

	if (this != &f)
	{

		// Remove old values.
		for (ui64 m = 0; m < this->values.size(); ++m)
			delete this->values[m];

		std::vector<std::string> mv = {};
		for (ui64 sindex = 0; sindex < f.values.size(); ++sindex)
		{
			std::string s = f.values[sindex];
			mv.push_back(s);
		}

		for (std::string s : mv)
		{
			char* sbuffer = new char[s.length() + 1];
			strcpy(sbuffer, s.c_str());
			this->values.push_back(sbuffer);
		}

	}

	return *this;

}

field::
~field()
{

	for (ui64 findex = 0; findex < this->values.size(); ++findex)
	{
		delete this->values[findex];
		this->values[findex] = nullptr;
	}

}

/**
 * Returns only the first field within a record, if such a field exists.
 */
std::string field::
get() const
{

	if (this->values.size() <= 0)
		return "";

	return this->values[0];

}

/**
 * Returns all the values within the field, seperated by the provided delimiter.
 */
std::string field::
get(std::string delim) const
{

	// Ensure we're not empty.
	if (this->values.size() <= 0)
		return "";

	// Construct the string.
	std::stringstream ss = {};
	for (ui64 sindex = 0; sindex < this->values.size(); ++sindex)
	{
		ss << this->values[sindex];
		if (sindex < this->values.size()-1)
			ss << delim;
	}

	// Return the string.
	return ss.str();

}

/**
 * Returns the given value from the field using the provided index.
 */
std::string field::
get(ui64 index) const
{

	// We must ensure that any index value is a valid index value,
	// even one is not given.
	if (index >= this->values.size())
		return "";

	return this->values[index];

}

std::string field::
operator[](ui64 index)
{
	// Alias of get, use that instead lol
	return this->get();
}

# Manage Kinetic

A conversion utility designed for converting the data from one ERP system to another
using Microsoft Excel as the transport layer.

## Using Manage Kinetic

In order to use Manage Kinetic, you must first have a delimited data source. This
source file can be defined to use any delimiter as long as the delimiter settings
match the ones defined within Manage Kinetic. By default, all fields within a given
record are delimited using a semicolon.

```john doe;my company;123 business laneýsuite 2;65432;california```

In order to meet the multivalue requirements, any fields with data seperated by
the character value of `0xFD` will be treated as multivalue fields. In the above
example record, the third field within contains a multivalue field which separates
address line one and address line two.

Once a data source is loaded, a table displaying the imported data source will be
shown. This table represents the parsed and split data source. Each column is given
a default heading such as "Column F0", "Column F1", and so on. These headings aren't
representative of their column names from the import and are for reference only.

Use the Table Mapping tab in the menu bar to open a "New Column Map" to begin mapping.
This will create a new window which displays all the available columns. Select the
column that you wish to map and apply the settings. A preview will be available which
shows how the map will look. The preview will show "Column F0 / Identifying Key" and
the mapped column. The "Column F0" serves as a reference and is assumed to be the
unique identifier from the import.

* <b>Alias Name:</b> Corresponds to the export column name.

* <b>Use all Multivalues?:</b> Checking this box will prompt the export to use all
multivalues in the export, separated by a delimiter.

* <b>Multivalue Delimiter:</b> If "Use all Multivalues?" is checked, this value will
be used to separate all given multivalues during export.

* <b>Multivalue Index:</b> This value determines which multivalue index to use during
export. In most cases, a value of one refers to the first value in the field, regardless
if the field itself is multivalue. A fields within a record are assumed multivalue, and
any given index within the field is assumed valid and will always return a value, valid
or empty. You can change this value to point to a different index for export.

In order to save the column map, you must provide an alias name. Once saved, it will
be available in the "Export Preview" tab on the main window.

## Build Manage Kinetic

In order to build Manage Kinetic, you will need CMake and Visual Studio. C++17 is
required in order to use OpenXLSX. There are no build dependencies, you simply just
need to run "build.ps1" or "build_release.ps1" and it should automatically run
configuration and build for you.

## System Requirements

Manage Kinetic runs on OpenGL and requires a machine with a "modern" CPU that can
support basic OpenGL. This means most PCs can run this program. However, the parsing
routine and export routines are memory intensive and require 1-2GB of RAM for moderate
to large datasets. For large datasets, 3-4GB may be required for concurrent export.
Most machines should still be able to run this program.

#include <managekinetic/commandline.h>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <windows.h>

// ---------------------------------------------------------------------------------------------------------------------
// CLI Argument Interface
// ---------------------------------------------------------------------------------------------------------------------

clinterface::
clinterface()
{

}

clinterface::
clinterface(int argc, char** argv)
{
	for (int i = 1; i < argc; ++i)
		this->insertArgument(argv[i], i-1);
}

argument* clinterface::
operator[](std::string key)
{
	for (argument& currentArgument : this->arguments)
		if (key == currentArgument.getKey())
			return &currentArgument;
	return nullptr;
}

argument* clinterface::
operator[](ui64 index)
{
	if (index < this->arguments.size() && index >= 0)
		return &this->arguments[index];
	return nullptr;
}

void clinterface::
insertArgument(std::string argumentString, ui64 index)
{
	argument currentArgument = {};
	currentArgument.setIndex(index);

	if (argumentString.length() >= 2 && argumentString[0] == '-')
	{
		if (argumentString.length() >= 3 && argumentString[1] == '-')
		{
			currentArgument.setArgumentType(argtype::parameter);
			currentArgument.setKey(argumentString);
			currentArgument.setValue(argumentString);
		}
		else
		{
			currentArgument.setArgumentType(argtype::flag);
			currentArgument.setKey(argumentString);
			currentArgument.setValue("true");
		}
	}

	else
	{
		currentArgument.setArgumentType(argtype::token);
		currentArgument.setKey(argumentString);
		currentArgument.setValue(argumentString);
	}

	this->arguments.push_back(std::move(currentArgument));
}

argument::argument()
{
	// Nothing to do.
}

argument::argument(argtype type, std::string keyValue)
{
	this->type = type;
	this->key = keyValue;
	this->value = keyValue;
}

argument::argument(argtype type, std::string key, std::string value)
{
	this->type = type;
	this->key = key;
	this->value = value;
}

void argument::
setArgumentType(argtype type)
{
	this->type = type;
}

void argument::
setValue(std::string value)
{
	this->value = value;
}

void argument::
setKey(std::string key)
{
	this->key = key;
}

argtype argument::
getArgumentType()
{
	return this->type;
}

std::string argument::
getValue()
{
	return this->value;
}

std::string argument::
getKey()
{
	return this->key;
}

ui64 argument::
getIndex()
{
	return this->index;
}

void argument::
setIndex(ui64 index)
{
	this->index = index;
}

bool argument::
isNumber()
{
	for (const char& character : this->value)
		if (!std::isdigit(character))
			return false;
	return true;
}

bool argument::
isPathValid()
{
	return std::filesystem::exists(this->value);
}

bool argument::
isFileExists()
{
	return std::filesystem::is_regular_file(this->value);
}

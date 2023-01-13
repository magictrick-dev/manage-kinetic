#ifndef MANAGE_KINETIC_COMMANDLINE_H
#define MANAGE_KINETIC_COMMANDLINE_H
#include <managekinetic/primitives.h>
#include <string>
#include <vector>

#define CLPARSE_RESULTS_VALID 					 0
#define CLPARSE_RESULTS_NOARGS 					-1
#define CLPARSE_RESULTS_NOEXIST 				-2


enum class argtype
{
	token,
	parameter,
	flag
};

class argument
{
	public:
		argument();
		argument(argtype type, std::string keyValue);
		argument(argtype type, std::string key, std::string value);

		void 		setKey(std::string key);
		void 		setValue(std::string value);
		void 		setIndex(ui64 index);
		void 		setArgumentType(argtype type);

		std::string getKey();
		std::string getValue();
		ui64 		getIndex();
		argtype 	getArgumentType();

		bool isNumber();
		bool isPathValid();
		bool isFileExists();

	private:
		argtype 	type;
		std::string key;
		std::string value;
		ui64 		index;
};

class clinterface
{
	public:
		clinterface();
		clinterface(int argc, char** argv);

		void insertArgument(std::string argument, ui64 index);

		argument* operator[](std::string key);
		argument* operator[](ui64 index);

	private:
		std::vector<argument> arguments;
};

#endif
#ifndef MANAGE_KINETIC_TABLEVIEWER_H
#define MANAGE_KINETIC_TABLEVIEWER_H
#include <managekinetic/primitives.h>
#include <managekinetic/application.h>
#include <vendor/DearImGUI/imgui.h>

class tableviewer
{

	public:
		tableviewer(std::string tablename);
		~tableviewer();

		void render(ui64 tableTotal, ui64 columnTotal);

		std::string getUniqueElementName(std::string elementName);

		// Setters

		inline void setColumnStep(ui32 step) // Note that ImGUI col count max is 64.
			{ if (step > 63) step = 63; else this->columnStep = step; }

		inline void setPaginationStep(ui32 step)
			{ this->paginationStep = step; }

		inline void setStartingOffset(ui64 offset)
			{ this->vOffset = offset; }

		// Getters

		inline ui32 getPaginationStep() const
			{ return this->paginationStep; }

		inline ui32 getColumnStep() const
			{ return this->columnStep; }

		inline ui64 getStartingOffset() const
			{ return this->vOffset; }

		inline bool isRowNumbersShown() const
			{ return this->showRowNumbers; }

	protected:
		virtual void headerProcedure(ui32 columnOffset, ui32 colCount);
		virtual void iterationProcedure(ui64 idx, ui32 columnOffset, ui32 colCount);

	private:
		i32 paginationStep 	= 100;
		i64 columnStep 		= 20;

		i64 vOffset 		= 0;
		i64 cOffset 		= 0;

		bool showRowNumbers 	= true;

	private:
		std::string tableName 	= "Data Table";
		ui32 instanceIdentifer 	= 0;
};

#endif
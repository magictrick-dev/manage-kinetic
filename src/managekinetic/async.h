#ifndef MANAGE_KINETIC_ASYNC_H
#define MANAGE_KINETIC_ASYNC_H
#include <managekinetic/primitives.h>
#include <thread>
#include <atomic>
#include <functional>

// ---------------------------------------------------------------------------------------------------------------------
// Operations & Routines
// ---------------------------------------------------------------------------------------------------------------------

class operation
{
	public:
		operation() 			= default;
		virtual ~operation() {};

		inline bool isStarted() const
			{ return this->operationStarted; }
		inline bool isComplete() const
			{ return this->operationCompleted; }

		virtual void run() 				= 0;
		
		void complete()
			{ this->operationCompleted = true; }


	protected:
		bool 	operationStarted 		= false;
		bool 	operationCompleted 		= false;

		void start()
			{ this->operationStarted = true; }

	private:
		ui32 	operationIdentifier 	= 0;

		void setIdentifier(ui32 identifier)
			{ this->operationIdentifier = identifier; };
		
		friend class routine_handler;
};

class routine_handler
{
	public:
		routine_handler() = default;
		~routine_handler();

		void 	executeOperations();

		ui32 	insertOperation(operation* operationPtr);
		void 	removeOperation(ui32 operationIdentifier);

		operation* getOperationFromIdentifier(ui32 operationIdentifier);

	private:
		ui32 					_operationIndexCounter = 0;
		std::vector<operation*> _operations;
};

// ---------------------------------------------------------------------------------------------------------------------
// Async Handle Interface
// ---------------------------------------------------------------------------------------------------------------------

class async_handle
{
	public:
		async_handle() 				{ _complete = false; }
		virtual ~async_handle() 	{ };
		
		bool isComplete() const 	{ return this->_complete; }
		void reset() 				{ this->_complete = false; }
		void complete() 			{ this->_complete = true; }

	protected:
		std::atomic<bool> _complete;
};


#endif
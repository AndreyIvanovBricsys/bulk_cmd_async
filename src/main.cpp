#include <iostream>
#include <thread>
#include "Context.h"

class CommandInterpreter
{
public:
	void interpret(const std::string & cmd, Context & context)
	{
		if (cmd == "{")
		{
			context.openScope();
		}
		else if (cmd == "}")
		{
			context.closeScope();
		}
		else
		{
			context.addCommand(Command{ cmd });
		}
	}
};

int main(int argc, char * argv[])
{
	if (argc < 2)
	{
		std::cout << "Static block size was not specified" << std::endl;
		return 1;
	}

	if (argc > 2)
	{

		try
		{
			const int staticBlockSize{ std::stoi(std::string{ argv[1] }) };

			{
				auto reactors{ std::make_unique<ReactorAggregation>() };
				reactors->addReactor(std::make_unique<ConsoleLogger>());
				reactors->addReactor(std::make_unique<FileLogger>());

				Context context{ static_cast<size_t>(staticBlockSize) };
				context.setReactor(std::move(reactors));

				CommandInterpreter interpreter;

				std::vector<std::string> input{ "cmd1", "cmd2", "cmd3", "cmd4", "cmd5" };

				for (const auto & line : input)
				{
					std::this_thread::sleep_for(std::chrono::seconds{ 1 });
					std::cout << "[" << line << "]" << std::endl;
					interpreter.interpret(line, context);
				}
			}

			std::cout << "===============================================" << std::endl;

			{
				auto reactors{ std::make_unique<ReactorAggregation>() };
				reactors->addReactor(std::make_unique<ConsoleLogger>());
				reactors->addReactor(std::make_unique<FileLogger>());

				Context context{ static_cast<size_t>(staticBlockSize) };
				context.setReactor(std::move(reactors));

				CommandInterpreter interpreter;

				std::vector<std::string> input{
					"cmd1", "cmd2",
					"{",
						"cmd3", "cmd4",
					"}",
					"{",
						"cmd5", "cmd6",
						"{",
							"cmd7", "cmd8",
						"}",
						"cmd9",
					"}",
					"{",
						"cmd10", "cmd11"
				};

				for (const auto & line : input)
				{
					std::this_thread::sleep_for(std::chrono::seconds{ 1 });
					std::cout << "[" << line << "]" << std::endl;
					interpreter.interpret(line, context);
				}
			}

			return 0;
		}
		catch (const std::exception & e)
		{
			std::cout << e.what() << std::endl;
			return 1;
		}
	}
	else
	{
		try
		{
			const int staticBlockSize{ std::stoi(std::string{ argv[1] }) };

			auto reactors{ std::make_unique<ReactorAggregation>() };
			reactors->addReactor(std::make_unique<ConsoleLogger>());
			reactors->addReactor(std::make_unique<FileLogger>());

			Context context{ static_cast<size_t>(staticBlockSize) };
			context.setReactor(std::move(reactors));

			CommandInterpreter interpreter;

			std::string line;
			while (std::getline(std::cin, line) && !line.empty())
			{
				interpreter.interpret(line, context);
			}
		}
		catch (const std::exception & e)
		{
			std::cout << e.what() << std::endl;
			return 1;
		}
	}
}

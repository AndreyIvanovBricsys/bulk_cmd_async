#include "Context.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <sstream>
#include <fstream>

Command::Command(const std::string & value) :
	m_value{ value }
{
}

void Command::execute() const
{
}

const std::string & Command::info() const
{
	return m_value;
}

void CommandBlock::addCommand(Command && command)
{

	m_commands.emplace_back(std::move(command));
}

size_t CommandBlock::numberOfCommands() const
{
	return m_commands.size();
}

void CommandBlock::execute() const
{
	for (const auto & command : m_commands)
	{
		command.execute();
	}
}

void CommandBlock::reset()
{
	m_commands.clear();
}

std::vector<std::string> CommandBlock::getCommandList() const
{
	std::vector<std::string> result;
	for (const auto & cmd : m_commands)
	{
		result.push_back(cmd.info());
	}
	return result;
}

void ContextState::addCommandImpl(Context & context, Command && command)
{
	context.currentCommandBlock().addCommand(std::move(command));

	auto reactor{ context.getReactor() };
	if (reactor)
	{
		reactor->onAddCommand(context.currentCommandBlock());
	}
}

void ContextState::executeBlockImpl(Context & context)
{
	auto reactor{ context.getReactor() };
	if (reactor)
	{
		reactor->onBlockExecute(context.currentCommandBlock());
	}

	context.currentCommandBlock().execute();
}

Context::Context(size_t staticBlockSize) :
	m_staticBlockMaxSize{ staticBlockSize }
{
	if (0 == m_staticBlockMaxSize)
	{
		throw std::runtime_error("Invalid static block size");
	}

	m_state = std::make_unique<BuildingStaticBlock>(*this);
}

Context::~Context()
{
	m_state->exit(*this);
}

void Context::setState(std::unique_ptr<ContextState> && state)
{
	m_state = std::move(state);
}

CommandBlock & Context::currentCommandBlock()
{
	return m_currentBlock;
}

size_t Context::staticBlockMaxSize() const
{
	return m_staticBlockMaxSize;
}

void Context::openScope()
{
	m_state->openScope(*this);
}

void Context::closeScope()
{
	m_state->closeScope(*this);
}

void Context::addCommand(Command && command)
{
	m_state->addCommand(*this, std::move(command));
}

void Context::setReactor(std::unique_ptr<Reactor> && reactor)
{
	m_reactor = std::move(reactor);
}

Reactor * Context::getReactor() const
{
	return m_reactor.get();
}

BuildingDynamicBlock::BuildingDynamicBlock(Context & context) :
	m_nestingLevel{ 1 }
{
	context.currentCommandBlock().reset();
}

void BuildingDynamicBlock::openScope(Context & context)
{
	++m_nestingLevel;
}

void BuildingDynamicBlock::closeScope(Context & context)
{
	if (0 == m_nestingLevel)
	{
		throw std::runtime_error("No scope was opened");
	}

	--m_nestingLevel;
	if (0 == m_nestingLevel)
	{
		executeBlockImpl(context);
		context.setState(std::make_unique<BuildingStaticBlock>(context));
	}
}

void BuildingDynamicBlock::addCommand(Context & context, Command && command) const
{
	addCommandImpl(context, std::move(command));
}

void BuildingDynamicBlock::exit(Context & context) const
{
}

BuildingStaticBlock::BuildingStaticBlock(Context & context)
{
	context.currentCommandBlock().reset();
}

void BuildingStaticBlock::openScope(Context & context)
{
	executeBlockImpl(context);
	context.setState(std::make_unique<BuildingDynamicBlock>(context));
}

void BuildingStaticBlock::closeScope(Context & context)
{
}

void BuildingStaticBlock::addCommand(Context & context, Command && command) const
{
	if (context.currentCommandBlock().numberOfCommands() < context.staticBlockMaxSize())
	{
		addCommandImpl(context, std::move(command));
	}

	if (context.currentCommandBlock().numberOfCommands() == context.staticBlockMaxSize())
	{
		executeBlockImpl(context);
		context.setState(std::make_unique<BuildingStaticBlock>(context));
	}
}

void BuildingStaticBlock::exit(Context & context) const
{
	executeBlockImpl(context);
}

void ReactorAggregation::addReactor(std::unique_ptr<Reactor> && reactor)
{
	if (reactor)
	{
		m_reactors.emplace_back(std::move(reactor));
	}
}

void ReactorAggregation::onBlockExecute(const CommandBlock & bulk)
{
	for (const auto & reactor : m_reactors)
	{
		reactor->onBlockExecute(bulk);
	}
}

void ReactorAggregation::onAddCommand(const CommandBlock & bulk)
{
	for (const auto & reactor : m_reactors)
	{
		reactor->onAddCommand(bulk);
	}
}

void ConsoleLogger::onBlockExecute(const CommandBlock & bulk)
{
	if (bulk.numberOfCommands() > 0)
	{
		std::cout << "bulk: ";

		const auto commandList{ bulk.getCommandList() };
		const auto lastCommand{ std::prev(commandList.cend()) };

		std::for_each(commandList.cbegin(), lastCommand,
			[](const std::string & command)
			{
				std::cout << command;
				std::cout << ", ";
			});

		std::cout << *lastCommand;
		std::cout << std::endl;
	}
}

void ConsoleLogger::onAddCommand(const CommandBlock &)
{
}

void FileLogger::onBlockExecute(const CommandBlock & bulk)
{
	if (bulk.numberOfCommands() > 0)
	{
		if (!m_blockInitTime)
		{
			throw std::runtime_error("Invalid block");
		}

		std::stringstream sstr;
		sstr << "bulk" << m_blockInitTime->count() << ".log";
		std::ofstream file{ sstr.str() };

		for (const auto & command : bulk.getCommandList())
		{
			file << command << std::endl;
		}

		m_blockInitTime.reset();
	}
}

void FileLogger::onAddCommand(const CommandBlock &)
{
	if (!m_blockInitTime)
	{
		m_blockInitTime = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch());
	}
}

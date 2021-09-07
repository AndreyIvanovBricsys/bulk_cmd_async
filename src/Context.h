#pragma once
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>

class Command
{
public:
	Command(const std::string & value);

	void execute() const;
	const std::string & info() const;

private:
	std::string m_value;
};

class CommandBlock
{
public:
	void addCommand(Command && command);
	size_t numberOfCommands() const;
	void execute() const;
	void reset();

	std::vector<std::string> getCommandList() const;

private:
	std::vector<Command> m_commands;
};

class Context;

class ContextState
{
public:
	virtual ~ContextState() = default;

	virtual void openScope(Context & context) = 0;
	virtual void closeScope(Context & context) = 0;
	virtual void addCommand(Context & context, Command && command) const = 0;
	virtual void exit(Context & context) const = 0;

protected:
	static void addCommandImpl(Context & context, Command && command);
	static void executeBlockImpl(Context & context);
};

class Reactor
{
public:
	virtual ~Reactor() = default;

	virtual void onBlockExecute(const CommandBlock &) = 0;
	virtual void onAddCommand(const CommandBlock &) = 0;
};

class Context
{
public:
	Context(size_t staticBlockSize);
	~Context();

	void setState(std::unique_ptr<ContextState> && state);
	CommandBlock & currentCommandBlock();
	size_t staticBlockMaxSize() const;

	void openScope();
	void closeScope();
	void addCommand(Command && command);

	void setReactor(std::unique_ptr<Reactor> && reactor);
	Reactor * getReactor() const;

private:
	std::unique_ptr<ContextState> m_state{ nullptr };
	std::unique_ptr<Reactor> m_reactor{ nullptr };
	CommandBlock m_currentBlock;
	size_t m_staticBlockMaxSize{ 1 };
};

class BuildingDynamicBlock : public ContextState
{
public:
	BuildingDynamicBlock(Context & context);
	BuildingDynamicBlock(const BuildingDynamicBlock &) = delete;

	void openScope(Context & context) override;
	void closeScope(Context & context) override;
	void addCommand(Context & context, Command && command) const override;
	void exit(Context & context) const override;

private:
	size_t m_nestingLevel{ 0 };
};

class BuildingStaticBlock : public ContextState
{
public:
	BuildingStaticBlock(Context & context);
	BuildingStaticBlock(const BuildingStaticBlock &) = delete;

	void openScope(Context & context) override;
	void closeScope(Context & context) override;
	void addCommand(Context & context, Command && command) const override;
	void exit(Context & context) const override;
};

class ReactorAggregation : public Reactor
{
public:
	void addReactor(std::unique_ptr<Reactor> && reactor);

	void onBlockExecute(const CommandBlock & bulk) override;
	void onAddCommand(const CommandBlock & bulk) override;

private:
	std::vector<std::unique_ptr<Reactor>> m_reactors;
};

class ConsoleLogger : public Reactor
{
public:
	void onBlockExecute(const CommandBlock & bulk) override;
	void onAddCommand(const CommandBlock &) override;
};

class FileLogger : public Reactor
{
public:
	void onBlockExecute(const CommandBlock & bulk) override;
	void onAddCommand(const CommandBlock &) override;

private:
	std::optional<std::chrono::seconds> m_blockInitTime{ std::nullopt };
};

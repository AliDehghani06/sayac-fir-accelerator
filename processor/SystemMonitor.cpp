#include <systemc.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <chrono>
#include "SystemTester.cpp"

class SystemMonitor
{
private:
    SystemTester *system_top;
    bool debug_enabled;
    bool exit_requested;

    // Command structure
    struct Command
    {
        std::string name;
        std::string description;
        std::function<void(const std::vector<std::string> &)> handler;
        std::vector<std::string> usage_examples;
    };

    std::map<std::string, Command> commands;

public:
    SystemMonitor(SystemTester *top) : system_top(top), debug_enabled(false), exit_requested(false)
    {
        initializeSystemDefaults();
        initializeCommands();
        printWelcome();
    }

    void run()
    {
        std::string input;

        while (!exit_requested)
        {
            std::cout << "\n\033[1;34mSAYAC>\033[0m ";
            std::getline(std::cin, input);

            if (input.empty())
                continue;

            processCommand(input);
        }
    }

private:
    void initializeSystemDefaults()
    {
        // Initialize memory module defaults
        // system_top->systemModule->memory->DebugON = 0;
        system_top->systemModule->memory->Loading = 0;
        system_top->systemModule->memory->StartingLocation = 0;
        system_top->systemModule->memory->PuttingData = 0;
        system_top->systemModule->memory->file = "";

        // Initialize instruction module defaults
        // system_top->systemModule->cpu->sayac->DebugON = 0;
    }

    void initializeCommands()
    {
        // Help command
        commands["help"] = {
            "help",
            "Show available commands or detailed help for a specific command",
            [this](const std::vector<std::string> &args)
            { handleHelp(args); },
            {"help", "help debug", "help load"}};

        // Debug commands
        commands["debug"] = {
            "debug",
            "Control debugging mode",
            [this](const std::vector<std::string> &args)
            { handleDebug(args); },
            {"debug on", "debug off", "debug status"}};

        // Load commands
        commands["load"] = {
            "load",
            "Load programs into memory",
            [this](const std::vector<std::string> &args)
            { handleLoad(args); },
            {"load binary filename.txt", "load assembly filename.asm", "load source filename.c"}};

        // Memory commands
        commands["memory"] = {
            "memory",
            "Memory operations",
            [this](const std::vector<std::string> &args)
            { handleMemory(args); },
            {"memory write", "memory set-start 100", "memory status"}};

        // Run command
        commands["run"] = {
            "run",
            "Execute the loaded program",
            [this](const std::vector<std::string> &args)
            { handleRun(args); },
            {"run", "run 5000"}};

        // Generate command
        commands["generate"] = {
            "generate",
            "Generate data files",
            [this](const std::vector<std::string> &args)
            { handleGenerate(args); },
            {"generate datafile.txt"}};

        // Exit command
        commands["exit"] = {
            "exit",
            "Exit the SAYAC interface",
            [this](const std::vector<std::string> &args)
            { handleExit(args); },
            {"exit", "quit"}};

        commands["quit"] = commands["exit"]; // Alias
    }

    void printWelcome()
    {
        std::cout << "\n\033[1;32m╔════════════════════════════════════════════════════════╗\033[0m\n";
        std::cout << "\033[1;32m║                     SAYAC Processor                    ║\033[0m\n";
        std::cout << "\033[1;32m║                Software User Interface                 ║\033[0m\n";
        std::cout << "\033[1;32m╚════════════════════════════════════════════════════════╝\033[0m\n";
        std::cout << "\nType '\033[1;33mhelp\033[0m' for available commands or '\033[1;33mexit\033[0m' to quit.\n";
    }

    void processCommand(const std::string &input)
    {
        auto args = parseCommand(input);
        if (args.empty())
            return;

        std::string cmd = toLowerCase(args[0]);

        auto it = commands.find(cmd);
        if (it != commands.end())
        {
            try
            {
                it->second.handler(args);
            }
            catch (const std::exception &e)
            {
                printError("Command execution failed: " + std::string(e.what()));
            }
        }
        else
        {
            printError("Unknown command: '" + cmd + "'. Type 'help' for available commands.");
        }
    }

    std::vector<std::string> parseCommand(const std::string &input)
    {
        std::vector<std::string> tokens;
        std::istringstream iss(input);
        std::string token;

        while (iss >> token)
        {
            tokens.push_back(token);
        }

        return tokens;
    }

    std::string toLowerCase(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // Command Handlers
    void handleHelp(const std::vector<std::string> &args)
    {
        if (args.size() == 1)
        {
            // General help
            std::cout << "\n\033[1;36mAvailable Commands:\033[0m\n";
            std::cout << "──────────────────────────────────────\n";

            for (const auto &[name, cmd] : commands)
            {
                if (name == cmd.name)
                { // Skip aliases
                    std::cout << std::left << std::setw(12) << name
                              << " - " << cmd.description << "\n";
                }
            }

            std::cout << "\nUse '\033[1;33mhelp <command>\033[0m' for detailed information about a specific command.\n";
        }
        else
        {
            // Specific command help
            std::string cmd = toLowerCase(args[1]);
            auto it = commands.find(cmd);

            if (it != commands.end())
            {
                const Command &command = it->second;
                std::cout << "\n\033[1;36m" << command.name << "\033[0m - " << command.description << "\n";
                std::cout << "\n\033[1;33mUsage examples:\033[0m\n";

                for (const auto &example : command.usage_examples)
                {
                    std::cout << "  " << example << "\n";
                }
            }
            else
            {
                printError("No help available for unknown command: " + cmd);
            }
        }
    }

    void handleDebug(const std::vector<std::string> &args)
    {
        if (args.size() == 1)
        {
            std::cout << "Debug mode is currently: "
                      << (debug_enabled ? "\033[1;32mENABLED\033[0m" : "\033[1;31mDISABLED\033[0m") << "\n";
            std::cout << "Use: debug on/off/status\n";
            return;
        }

        std::string action = toLowerCase(args[1]);

        if (action == "on" || action == "enable")
        {
            debug_enabled = true;
            // system_top->systemModule->cpu->sayac->DebugON = 1;
            // system_top->systemModule->memory->DebugON = 1;
            printSuccess("Debug mode enabled");
        }
        else if (action == "off" || action == "disable")
        {
            debug_enabled = false;
            // system_top->systemModule->cpu->sayac->DebugON = 0;
            // system_top->systemModule->memory->DebugON = 0;
            printSuccess("Debug mode disabled");
        }
        else if (action == "status")
        {
            std::cout << "Debug status: "
                      << (debug_enabled ? "\033[1;32mENABLED\033[0m" : "\033[1;31mDISABLED\033[0m") << "\n";
        }
        else
        {
            printError("Invalid debug option. Use: on, off, or status");
        }
    }

    void handleLoad(const std::vector<std::string> &args)
    {
        if (args.size() < 3)
        {
            printError("Usage: load <type> <filename>");
            std::cout << "Types: binary, assembly, source\n";
            return;
        }

        std::string type = toLowerCase(args[1]);
        std::string filename = args[2];

        if (type == "binary" || type == "bin")
        {
            loadBinaryFile(filename);
        }
        else if (type == "assembly" || type == "asm")
        {
            loadAssemblyFile(filename);
        }
        else if (type == "source" || type == "src" || type == "c")
        {
            loadSourceFile(filename);
        }
        else
        {
            printError("Unknown file type: " + type);
            std::cout << "Supported types: binary, assembly, source\n";
        }
    }

    void handleMemory(const std::vector<std::string> &args)
    {
        if (args.size() == 1)
        {
            std::cout << "Memory commands:\n";
            std::cout << "  memory write     - Manually write data to memory\n";
            std::cout << "  memory set-start <addr> - Set program starting address\n";
            std::cout << "  memory status    - Show memory status\n";
            return;
        }

        std::string action = toLowerCase(args[1]);

        if (action == "write")
        {
            handleManualMemoryWrite();
        }
        else if (action == "set-start" && args.size() >= 3)
        {
            try
            {
                int start_addr = std::stoi(args[2]);
                system_top->systemModule->memory->StartingLocation = start_addr;
                printSuccess("Program starting address set to: " + std::to_string(start_addr));
            }
            catch (const std::exception &)
            {
                printError("Invalid address format");
            }
        }
        else if (action == "status")
        {
            std::cout << "Memory Status:\n";
            std::cout << "  Starting Location: "
                      << system_top->systemModule->memory->StartingLocation << "\n";
            // std::cout << "  Debug Mode: "
            //          << (system_top->systemModule->memory->DebugON ? "ON" : "OFF") << "\n";
            std::cout << "  Current File: "
                      << (system_top->systemModule->memory->file.empty() ? "None" : system_top->systemModule->memory->file) << "\n";
        }
        else
        {
            printError("Unknown memory command: " + action);
        }
    }

    void handleRun(const std::vector<std::string> &args)
    {
        int runtime = 7000; // Default runtime

        if (args.size() >= 2)
        {
            try
            {
                runtime = std::stoi(args[1]);
            }
            catch (const std::exception &)
            {
                printWarning("Invalid runtime value, using default (7000 NS)");
            }
        }

        std::cout << "\033[1;32mRunning simulation for " << runtime << " NS...\033[0m\n";

        auto start_time = std::chrono::high_resolution_clock::now();
        sc_start(runtime, SC_NS);
        auto end_time = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        printSuccess("Simulation completed in " + std::to_string(duration.count()) + " ms");
    }

    void handleGenerate(const std::vector<std::string> &args)
    {
        if (args.size() < 2)
        {
            printError("Usage: generate <filename>");
            return;
        }

        std::string filename = args[1];
        generateDataFile(filename);
    }

    void handleExit(const std::vector<std::string> &args)
    {
        std::cout << "\n\033[1;33mThank you for using SAYAC System Interface!\033[0m\n";
        std::cout << "Goodbye!\n";
        exit_requested = true;
    }

    // Utility functions
    bool fileExists(const std::string &filename)
    {
        struct stat buffer;
        return (stat(filename.c_str(), &buffer) == 0);
    }

    void loadBinaryFile(const std::string &filename)
    {
        std::string full_filename = filename;
        if (filename.find(".txt") == std::string::npos)
        {
            full_filename += ".txt";
        }

        if (!fileExists(full_filename))
        {
            printError("Binary file not found: " + full_filename);
            return;
        }

        // Reset flags first
        system_top->systemModule->memory->Loading = 0;
        system_top->systemModule->memory->PuttingData = 0;

        // Set new parameters
        system_top->systemModule->memory->file = full_filename;
        system_top->systemModule->memory->Loading = 1;

        printSuccess("Binary file set for loading: " + full_filename);
        printWarning("Run the simulation to load the file into memory");
    }

    void loadAssemblyFile(const std::string &filename)
    {
        std::string full_filename = filename;
        //if (filename.find(".asm") == std::string::npos)
        //{
        //    full_filename += ".asm";
        //}

        if (!fileExists(full_filename))
        {
            printError("Assembly file not found: " + full_filename);
            return;
        }

        if (!fileExists("./SAYACasm"))
        {
            printError("Assembler './SAYACasm' not found!");
            return;
        }

        std::cout << "Assembling " << full_filename << "...\n";

        if (executeCommand("./SAYACasm", {full_filename}))
        {
            if (fileExists("binfile.txt"))
            {
                // Reset flags first
                system_top->systemModule->memory->Loading = 0;
                system_top->systemModule->memory->PuttingData = 0;

                // Set new parameters
                system_top->systemModule->memory->file = "binfile.txt";
                system_top->systemModule->memory->Loading = 1;

                printSuccess("Assembly file assembled and set for loading");
                printWarning("Run the simulation to load the assembled code into memory");
            }
            else
            {
                printError("Assembly succeeded but binary file not created");
            }
        }
        else
        {
            printError("Assembly failed");
        }
    }

    void loadSourceFile(const std::string &filename)
    {
        std::string full_filename = filename;
        if (filename.find(".c") == std::string::npos)
        {
            full_filename += ".c";
        }

        if (!fileExists(full_filename))
        {
            printError("Source file not found: " + full_filename);
            return;
        }

        if (!fileExists("./SAYACcmp"))
        {
            printError("C compiler './SAYACcmp' not found!");
            printWarning("Please ensure the compiler is installed and accessible");
            return;
        }

        if (!fileExists("./SAYACasm"))
        {
            printError("Assembler './SAYACasm' not found!");
            printWarning("Please ensure the assembler is installed and accessible");
            return;
        }

        // Generate assembly filename
        std::string asm_filename = full_filename.substr(0, full_filename.find_last_of(".")) + ".asm";

        std::cout << "Compiling C file to assembly...\n";

        // Compile C to assembly
        if (executeCommand("./SAYACcmp", {full_filename, "-o", asm_filename}))
        {
            if (fileExists(asm_filename))
            {
                std::cout << "C compilation done. Now assembling...\n";

                // Assemble to binary
                if (executeCommand("./SAYACasm", {asm_filename}))
                {
                    if (fileExists("binfile.txt"))
                    {
                        // Reset flags first
                        system_top->systemModule->memory->Loading = 0;
                        system_top->systemModule->memory->PuttingData = 0;

                        // Set new parameters
                        system_top->systemModule->memory->file = "binfile.txt";
                        system_top->systemModule->memory->Loading = 1;

                        printSuccess("Source file compiled, assembled and set for loading");
                        printWarning("Run the simulation to load the compiled code into memory");
                    }
                    else
                    {
                        printError("Assembly succeeded but binary file not created");
                    }
                }
                else
                {
                    printError("Assembly process failed");
                }
            }
            else
            {
                printError("C compilation succeeded but assembly file not created");
            }
        }
        else
        {
            printError("C compilation failed");
        }
    }

    bool executeCommand(const std::string &command, const std::vector<std::string> &args)
    {
        std::vector<char *> argv;
        argv.push_back(const_cast<char *>(command.c_str()));

        for (const auto &arg : args)
        {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(nullptr);

        pid_t pid = fork();
        if (pid == 0)
        {
            execvp(command.c_str(), argv.data());
            _exit(1); // Use _exit instead of std::exit
        }
        else if (pid > 0)
        {
            int status;
            waitpid(pid, &status, 0);
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }

        return false;
    }

    void handleManualMemoryWrite()
    {
        std::cout << "\n\033[1;36mManual Memory Write Mode\033[0m\n";
        std::cout << "Enter data in format: <address> <data>\n";
        std::cout << "Type '.' on a new line to finish\n";
        std::cout << "────────────────────────────────────\n";

        std::ofstream addr_file("addr.txt");
        std::ofstream data_file("data.txt");

        if (!addr_file || !data_file)
        {
            printError("Could not create temporary files");
            return;
        }

        std::string input;
        int entries = 0;
        while (true)
        {
            std::cout << "WRITE> ";
            std::getline(std::cin, input);

            if (input == ".")
                break;

            std::istringstream iss(input);
            std::string addr_str, data_str;

            if (iss >> addr_str >> data_str)
            {
                addr_file << addr_str << "\n";
                data_file << data_str << "\n";
                std::cout << "Added: " << addr_str << " -> " << data_str << "\n";
                entries++;
            }
            else if (!input.empty())
            {
                printWarning("Invalid format. Use: <address> <data>");
            }
        }

        addr_file.close();
        data_file.close();

        if (entries > 0)
        {
            // Reset flags first
            system_top->systemModule->memory->Loading = 0;
            system_top->systemModule->memory->PuttingData = 0;

            // Set new parameters
            system_top->systemModule->memory->PuttingData = 1;

            printSuccess("Added " + std::to_string(entries) + " memory entries");
            printWarning("Run the simulation to write the data into memory");
        }
        else
        {
            printWarning("No data entered");
        }
    }

    void generateDataFile(const std::string &filename)
    {
        std::cout << "\n\033[1;36mData File Generation Mode\033[0m\n";
        std::cout << "Enter data in format: <address> <data>\n";
        std::cout << "Type '.' on a new line to finish\n";
        std::cout << "────────────────────────────────────\n";

        std::ofstream file(filename);
        if (!file)
        {
            printError("Could not create file: " + filename);
            return;
        }

        std::map<int, std::string> data_map;

        std::string input;
        while (true)
        {
            std::cout << "GEN> ";
            std::getline(std::cin, input);

            if (input == ".")
                break;

            std::istringstream iss(input);
            std::string addr_str, data_str;

            if (iss >> addr_str >> data_str)
            {
                try
                {
                    int addr = std::stoi(addr_str);
                    data_map[addr] = data_str;
                    std::cout << "Added: " << addr << " -> " << data_str << "\n";
                }
                catch (const std::exception &)
                {
                    printWarning("Invalid address format: " + addr_str);
                }
            }
            else if (!input.empty())
            {
                printWarning("Invalid format. Use: <address> <data>");
            }
        }

        // Write sorted data to file
        for (const auto &[addr, data] : data_map)
        {
            file << data << "\n";
        }

        file.close();
        printSuccess("Data file generated: " + filename + " (" + std::to_string(data_map.size()) + " entries)");
    }

    // Output formatting functions
    void printSuccess(const std::string &message)
    {
        std::cout << "\033[1;32m" << message << "\033[0m\n";
    }

    void printError(const std::string &message)
    {
        std::cout << "\033[1;31mError: " << message << "\033[0m\n";
    }

    void printWarning(const std::string &message)
    {
        std::cout << "\033[1;33mWarning: " << message << "\033[0m\n";
    }
};

// Updated main functions
void SoftwareUserInterface(SystemTester *TOP);

int sc_main(int argc, char *argv[])
{
    sc_report_handler::set_actions(SC_ID_VECTOR_CONTAINS_LOGIC_VALUE_, SC_DO_NOTHING);
    sc_report_handler::set_actions(SC_WARNING, SC_DO_NOTHING);

    SystemTester *TOP = new SystemTester("SystemTester_TB");
    sc_trace_file *VCDFile;
    VCDFile = sc_create_vcd_trace_file("wave");
    sc_trace(VCDFile, TOP->clk, "clk");
    sc_trace(VCDFile, TOP->systemModule->mma->controlReg, "mmaControlReg");
    sc_trace(VCDFile, TOP->systemModule->mma->dmaOut, "mmaDMAOut");
    sc_trace(VCDFile, TOP->systemModule->mma->dmaIn, "mmaDMAIn");
    sc_trace(VCDFile, TOP->systemModule->mma->interrupt, "mmaInterrupt");

    sc_trace(VCDFile, TOP->systemModule->bus->req, "busReq");



    sc_trace(VCDFile, TOP->systemModule->dma->t_cs, "dmaConfigPortCS");
    sc_trace(VCDFile, TOP->systemModule->dma->t_in, "dmaConfigPortIn");
    sc_trace(VCDFile, TOP->systemModule->dma->t_out, "dmaConfigPortOut");
    sc_trace(VCDFile, TOP->systemModule->dma->t_addr, " dmaConfigPortAddr");
    sc_trace(VCDFile, TOP->systemModule->dma->t_rd, "dmaConfigPortRD");
    sc_trace(VCDFile, TOP->systemModule->dma->t_wr, "dmaConfigPortWR");
    sc_trace(VCDFile, TOP->systemModule->dma->t_ready, "dmaConfigPortReady");

    sc_trace(VCDFile, TOP->systemModule->memory->t_cs, " memPortCS");
    sc_trace(VCDFile, TOP->systemModule->memory->t_in, " memPortIn");
    sc_trace(VCDFile, TOP->systemModule->memory->t_out, " memPortOut");
    sc_trace(VCDFile, TOP->systemModule->memory->t_addr, " memPortAddr");
    sc_trace(VCDFile, TOP->systemModule->memory->t_rd, " memPortRD");
    sc_trace(VCDFile, TOP->systemModule->memory->t_wr, " memPortWR");
    sc_trace(VCDFile, TOP->systemModule->memory->t_ready, " memPortReady");

    sc_trace(VCDFile, TOP->systemModule->mma->t_cs, " MMAPortCS");
    sc_trace(VCDFile, TOP->systemModule->mma->t_in, " MMAPortIn");
    sc_trace(VCDFile, TOP->systemModule->mma->t_out, " MMAPortOut");
    sc_trace(VCDFile, TOP->systemModule->mma->t_addr, " MMAPortAddr");
    sc_trace(VCDFile, TOP->systemModule->mma->t_rd, " MMAPortRD");
    sc_trace(VCDFile, TOP->systemModule->mma->t_wr, " MMAPortWR");
    sc_trace(VCDFile, TOP->systemModule->mma->t_ready, " MMAPortReady");

    sc_trace(VCDFile, TOP->systemModule->cpu->i_in, "cpuInitiatorIn");
    sc_trace(VCDFile, TOP->systemModule->cpu->i_addr, "cpuInitiatorAddr");
    sc_trace(VCDFile, TOP->systemModule->cpu->i_out, "cpuInitiatorOut");
    sc_trace(VCDFile, TOP->systemModule->cpu->i_rd, "cpuInitiatorRD");
    sc_trace(VCDFile, TOP->systemModule->cpu->i_wr, "cpuInitiatorWR");
    sc_trace(VCDFile, TOP->systemModule->cpu->i_ready, "cpuInitiatorReady");

    // sc_trace(VCDFile, TOP->systemModule->dp->i_in, "cpuInitiatorIn");
    // sc_trace(VCDFile, TOP->systemModule->dp->i_addr, "cpuInitiatorAddr");
    // sc_trace(VCDFile, TOP->systemModule->dp->i_out, "cpuInitiatorOut");
    // sc_trace(VCDFile, TOP->systemModule->dp->i_rd, "cpuInitiatorRD");
    // sc_trace(VCDFile, TOP->systemModule->dp->i_wr, "cpuInitiatorWR");
    // sc_trace(VCDFile, TOP->systemModule->dp->i_ready, "cpuInitiatorReady");

    sc_trace(VCDFile, TOP->systemModule->dma->interrupt, "dmaInterrupt");
    sc_trace(VCDFile, TOP->systemModule->dma->i_in, "dmaInitiatorIn");
    sc_trace(VCDFile, TOP->systemModule->dma->i_addr, "dmaInitiatorAddr");
    sc_trace(VCDFile, TOP->systemModule->dma->i_out, "dmaInitiatorOut");
    sc_trace(VCDFile, TOP->systemModule->dma->i_rd, "dmaInitiatorRD");
    sc_trace(VCDFile, TOP->systemModule->dma->i_wr, "dmaInitiatorWR");
    sc_trace(VCDFile, TOP->systemModule->dma->i_ready, "dmaInitiatorReady");
    sc_trace(VCDFile, TOP->systemModule->dma->controlReg, "dmaControlReg");
    sc_trace(VCDFile, TOP->systemModule->dma->byteCount, "dmaByteCount");
    sc_trace(VCDFile, TOP->systemModule->dma->fromAddress, "dmaFromAddress");
    sc_trace(VCDFile, TOP->systemModule->dma->toAddress, "dmaToAddress");
   


    sc_trace(VCDFile, TOP->systemModule->pic->INT, "picINT");
    sc_trace(VCDFile, TOP->systemModule->pic->PIC_output, "picOUT");
    sc_trace(VCDFile, TOP->systemModule->pic->IR, "picIR");

    for (int i = 0; i < 2; i++)
    {
        sc_trace(VCDFile, TOP->systemModule->bus->req[i], "busInitiatorReq" + std::to_string(i));
        sc_trace(VCDFile, TOP->systemModule->bus->gnt[i], "busInitiatorGnt" + std::to_string(i));
        sc_trace(VCDFile, TOP->systemModule->bus->i_in[i], "busInitiatorIn" + std::to_string(i));
        sc_trace(VCDFile, TOP->systemModule->bus->i_addr[i], "busInitiatorAddr" + std::to_string(i));
        sc_trace(VCDFile, TOP->systemModule->bus->i_out[i], "busInitiatorOut" + std::to_string(i));
        sc_trace(VCDFile, TOP->systemModule->bus->i_rd[i], "busInitiatorRD" + std::to_string(i));
        sc_trace(VCDFile, TOP->systemModule->bus->i_wr[i], "busInitiatorWR" + std::to_string(i));
        sc_trace(VCDFile, TOP->systemModule->bus->i_ready[i], "busInitiatorReady" + std::to_string(i));
    }

    for (int i = 0;i < 4;i++)
    {
        sc_trace(VCDFile, TOP->systemModule->bus->cs[i], "busTargetCS" + std::to_string(i));
    }
    sc_trace(VCDFile, TOP->systemModule->bus->addressBus, "busAddressBus");
    sc_trace(VCDFile, TOP->systemModule->bus->dataBus, "busDataBus");
    sc_trace(VCDFile, TOP->systemModule->bus->targetReadyBus, "busTargetReadyBus");

    sc_trace(VCDFile, TOP->systemModule->cpu->sayac->PCregister, "sayacPC");

    SoftwareUserInterface(TOP);

    return 0;
}

void SoftwareUserInterface(SystemTester *TOP)
{
    SystemMonitor interface(TOP);
    interface.run();
}
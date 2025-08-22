#pragma once

#include "allocator.h"
#include "array.h"
#include "def.h"
#include "string.h"
#include <optional>
#include <print>

struct CLIOption {
    string short_name;
    string long_name;
    string description;
    std::optional<string> value;
    bool flag_option;

    static CLIOption init(
        string short_name,
        string long_name,
        string description,
        bool flag_option = false
    ) {
        return CLIOption{
            .short_name = short_name,
            .long_name = long_name,
            .description = description,
            .value = std::nullopt,
            .flag_option = flag_option,
        };
    }

    static string parse_name(string arg) {
        if (arg[0] == '-' && arg[1] == '-') {
            return arg + 2; // Skip "--"
        } else if (arg[0] == '-') {
            return arg + 1; // Skip "-"
        }

        return arg;
    }

    bool equals(string name_short_or_long) {
        bool eq_short = string_equals(parse_name(short_name), name_short_or_long);
        bool eq_long = string_equals(parse_name(long_name), name_short_or_long);

        return (short_name && eq_short) || (long_name && eq_long);
    }
};

struct CLICommand;

typedef bool (*CommandCallback)(CLICommand& command, void* user_data);

struct CLICommand {
    string name;
    string description;
    ArrayList<CLIOption> options;
    CommandCallback callback;
    void* user_data;

    static CLICommand init(
        Allocator& allocator,
        string name,
        string description,
        CommandCallback callback = nullptr,
        void* user_data = nullptr,
        usize max_options = 10
    ) {
        auto options = ArrayList<CLIOption>::init(allocator, max_options);

        return CLICommand{
            .name = name,
            .description = description,
            .options = options,
            .callback = callback,
            .user_data = user_data,
        };
    }

    void deinit() { options.deinit(); }

    bool add_option(CLIOption& option) { return options.append(option); }

    std::optional<CLIOption> get_option(string name) {
        for (auto& opt : options) {
            if (opt.equals(name)) return opt;
        }
        return std::nullopt;
    }

    void set_option_value(string name, string value) {
        for (auto& opt : options) {
            if (opt.equals(name)) {
                opt.value = value;
                return;
            }
        }
    }

    bool execute() {
        if (callback) {
            return callback(*this, user_data);
        }
        return true;
    }
};

struct CLIParser {
    string program_name;
    std::optional<CLICommand> current_command;
    std::optional<CLICommand> main_command;
    ArrayList<CLICommand> commands;

    static CLIParser init(Allocator& allocator, string program_name, i32 max_commands = 20) {
        auto commands = ArrayList<CLICommand>::init(allocator, max_commands);

        return CLIParser{
            .program_name = program_name,
            .current_command = std::nullopt,
            .main_command = std::nullopt,
            .commands = commands,
        };
    }

    void deinit() {
        for (auto& cmd : commands) {
            cmd.deinit();
        }

        if (main_command.has_value()) {
            main_command->deinit();
        }

        commands.deinit();
    }

    bool add_command(CLICommand command) { return commands.append(command); }

    void set_main_command(CLICommand command) { main_command = command; }

    int parse_and_execute(i32 argc, char* argv[]) {
        if (!parse(argc, argv)) {
            return 1;
        }

        if (current_command && current_command->callback) {
            return current_command->execute() ? 0 : 1;
        }

        return 0;
    }

    bool parse(i32 argc, char* argv[]) {
        // If no arguments or first argument is an option, use main command
        if (argc < 2 || is_option(argv[1])) {
            if (!main_command.has_value()) {
                print_help();
                return false;
            }

            current_command = main_command;

            // Parse options starting from argv[1] for main command
            for (i32 i = 1; i < argc; i++) {
                if (string_equals(argv[i], "--help") || string_equals(argv[i], "-h")) {
                    print_command_help(current_command.value());
                    return false;
                }

                if (is_option(argv[i])) {
                    i32 consumed = parse_option(argv, argc, i, current_command.value());
                    if(consumed == -1) return false;

                    i += consumed;
                }
            }

            return true;
        }

        // Look for named command
        auto found_command = find_command(argv[1]);
        if (!found_command.has_value()) {
            std::println("Unknown command: {}", argv[1]);
            print_help();
            return false;
        }

        current_command = found_command;

        // Check for help option
        for (i32 i = 2; i < argc; i++) {
            if (string_equals(argv[i], "--help") || string_equals(argv[i], "-h")) {
                print_command_help(current_command.value());
                return false;
            }
        }

        // Parse options starting from argv[2] for named commands
        for (i32 i = 2; i < argc; i++) {
            if (is_option(argv[i])) {
                i32 consumed = parse_option(argv, i, argc, current_command.value());
                if (consumed == -1) {
                    return false;
                }
                i += consumed;
            }
        }

        return true;
    }

    void print_help() {
        std::println("Usage {} [options]", program_name);

        if (main_command.has_value()) {
            std::println("       {} <command> [options]\n", program_name);
        } else {
            std::println("       {} <command> [options]\n", program_name);
        }

        if (main_command.has_value()) {
            std::println("Main command:");
            std::println("{:<15} {}\n", "(default)", main_command->description);
        }

        if (commands.len > 0) {
            std::println("Commands:");
            for (auto& cmd : commands) {
                std::println("{:<15} {}", cmd.name, cmd.description);
            }
        }

        std::println(
            "\nUse '{} --help' or '{} <command> --help' for more information.\n",
            program_name,
            program_name
        );
    }

    void print_command_help(CLICommand& command) {
        std::println("Usage: {} {} [options]\n", program_name, command.name);
        std::println("{}\n", command.description);

        if (command.options.len > 0) {
            std::println("Options:");

            for (auto& option : command.options) {
                std::print("  ");

                if (option.short_name) {
                    std::print("-{}", option.short_name);
                    if (option.long_name) {
                        std::print(", ");
                    }
                }

                if (option.long_name) {
                    std::print("--{}", option.long_name);
                }

                if (option.value.has_value()) {
                    std::print(" <value>");
                }

                std::println("\n{}", option.description);
            }
        }
    }

  private:
    i32 parse_option(char* argv[], i32 argc, i32 current_index, CLICommand& command) {
        string option_name = CLIOption::parse_name(argv[current_index]);

        // Find the option in the command
        for (auto& option : command.options) {
            if (option.equals(option_name)) {
                if (option.flag_option) {
                    command.set_option_value(option_name, "true"); // Flag options when present in the args are always `true`
                    return 0; // No additional arguments consumed
                }
                // Option expects a value
                if(current_index + 1 < argc && !is_option(argv[current_index + 1])) {
                    command.set_option_value(option_name, argv[current_index + 1]);
                    return 1; // One additional argument consumed
                } else {
                    std::println("Option {} requires a value.", argv[current_index]);
                    return -1;
                }
            }
        }

        std::println("Unknown option: {}", argv[current_index]);
        return -1; // Error
    }

    std::optional<CLICommand> find_command(string name) {
        for (auto& command : commands) {
            if (string_equals(command.name, name)) {
                return command;
            }
        }

        return std::nullopt;
    }

    bool is_option(char* arg) { return arg[0] == '-'; }
};

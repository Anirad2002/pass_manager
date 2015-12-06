#include "application.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace pass_manager {

Application::Application(const std::string& pass, const std::string& file)
    : file_(pass, file), manager_(), quit_(false), initialized_(false), actions_() {

    using namespace std::placeholders;

    try {
        initialize();
        initialized_ = true;
    } catch (std::ios::failure& e) {
        std::cerr
            << "Error reading '" << file << "', "
            << "probably incorrect password or file was currupted."
            << std::endl;
    }

    register_action("quit", "terminates application and saves state",
                    std::bind(&Application::quit, this, _1, _2));
    register_action("q", "alias for quit",
                    std::bind(&Application::quit, this, _1, _2));
    register_action("list", "prints possible actions",
                    std::bind(&Application::list, this, _1, _2));
    register_action("ls", "alias for list",
                    std::bind(&Application::list, this, _1, _2));
    register_action("?", "alias for list",
                    std::bind(&Application::list, this, _1, _2));
}

Application::~Application() {
    if (!initialized_) { return; }

    auto& output = file_.output();
    for (const Record& record : manager_) {
        record.dump(output);
    }
}

void Application::loop() {
    while (!quit_) {
        std::cout << "> ";
        std::string action;
        std::cin >> action;
        std::string str_arguments;
        std::getline(std::cin, str_arguments);
        if (!std::cin.good()) { return; }
        std::istringstream arguments_stream(str_arguments);
        std::vector<std::string> arguments;
        std::copy(std::istream_iterator<std::string>(arguments_stream), {},
                  std::back_inserter(arguments));
        get_action(action)(manager_, arguments);
    }
}

void Application::register_action(const std::string& name,
                                  const std::string& description,
                                  raw_action new_action) {
    actions_.insert(std::make_pair(name, std::make_pair(description, new_action)));
}

void Application::initialize() {
    try {
        auto& input = file_.input();
        while(true) {
            Record record = Record::load(input);
            if (input.eof()) { return; }
            manager_.add_record(record);
        }
    } catch (...) {
        file_.reset();
        throw;
    }
}

void Application::quit(Manager&, const arguments&) {
    quit_ = true;
}

void Application::list(Manager&, const arguments&) {
    for (auto& action : actions_) {
        std::cout << "    " << std::setw(20) << std::left << action.first << " "
                  << action.second.first << "\n";
    }
    std::cout << std::endl;
}

void Application::no_action(Manager& m, const arguments& a) {
    std::cout << "Unrecognized action. Possilbe actions: " << std::endl;
    list(m, a);
}

Application::raw_action Application::get_action(const std::string& name) {
    auto action = actions_.find(name);
    if (action == actions_.end()) {
        using namespace std::placeholders;
        return std::bind(&Application::no_action, this, _1, _2);
    } else {
        return action->second.second;
    }
}

}


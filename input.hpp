#ifndef _INPUT_HPP_
#define _INPUT_HPP_

#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#ifdef __linux__ 
    #include <termios.h>
#elif _WIN32
    #include <conio.h>
#else
    throw std::exception;
#endif

namespace in {
    using Check = std::function<void(std::string)>;
    using InputFunction = std::function<std::string()>;

    const char* INVALID_INPUT_PROMPT = "Invalid Input";
    const char INPUT_MASK = '*';

    enum class Style {
        Basic,
        Masked,
        Instant
    };

    class InvalidInputException : public std::exception {
    public:
        InvalidInputException(const char* error_message = INVALID_INPUT_PROMPT) : error_message(error_message) {
            /*
            Constructor.

            Arguments
            --------------------
                Error Message | const char* | The error message to show the user
            */
        }

        const char* what() const throw() {
            return this->error_message;
        }

    private:
        const char* error_message;
    };

    namespace _infuncs {
        char getch() {
            /*
            Get a single character from the user without requiring them to press the ENTER key to confirm

            Returns
            --------------------
                Character | char | The character pressed
            */

            char input;

            #ifdef __linux__
                termios terminal_flags;

                tcgetattr(0, &terminal_flags);
                terminal_flags.c_lflag &= ~ICANON;
                terminal_flags.c_lflag &= ~ECHO;
                tcsetattr(0, TCSANOW, &terminal_flags);
                
                if (!std::cin) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                }
                input = getchar();

                terminal_flags.c_lflag |= ICANON;
                terminal_flags.c_lflag |= ECHO;
                tcsetattr(0, TCSANOW, &terminal_flags);
            #elif _WIN32
                input = _getch();
            #endif

            return input;
        }
    }

    namespace infuncs {
        std::string basic_input(std::optional<char> mask = std::nullopt) {
            /*
            An input function that takes in input of any length from the user

            Arguments
            --------------------
                Mask (NULL) | char | A mask to apply to the character's input (if any)

            Returns
            --------------------
                User Input | std::string | The user's input
            */

            #ifdef __linux__
                const char NEWLINE = 10;
                const char BACKSPACE = 127;
                const char CURSOR_BACK = 8;
            #elif _WIN32
                const char NEWLINE = '\r';
                const char BACKSPACE = '\b';
                const char CURSOR_BACK = '\b';
            #endif

            std::string user_input;
            char character;

            do {
                character = _infuncs::getch();

                switch (character) {
                case BACKSPACE:
                    if (!user_input.empty()) {
                        user_input.pop_back();
                        std::cout << CURSOR_BACK << " " << CURSOR_BACK; // Replace the last character that was printed to STDOUT with a space
                    }
                    break;

                default:
                    // Make sure the key isn't ENTER
                    if (character != NEWLINE) {
                        user_input += character;
                        std::cout << (mask ? mask.value() : character);
                    }
                }
            } while (character != NEWLINE);

            return user_input;
        }

        std::string instant_input() {
            /*
            An input function that takes in a single character from the user 
            
            Returns
            --------------------
                User Input | std::string | The user's input

            Notes
            --------------------
                This function will NOT echo the user's input
            */

            std::string user_input;
            
            char character = _infuncs::getch();
            user_input += character;

            std::cout << char(toupper(character)) << std::flush;

            return user_input;
        }
    }

    template <class... Checks>
    std::string validate(InputFunction input_function, Checks... checks) {
        /*
        A wrapper function to apply checks to a generic input function.

        Arguments
        --------------------
            Input Function | std::string(*)()     | The input function to use
            Check...       | bool(*)(std::string) | A predicate to call on the user's input to determine if it's "valid" or not

        Returns
        --------------------
            User Input | std::string | The user's input that was returned from the input function 
        */

        std::string user_input;

        // Only attempt to validate if there were checks passed in
        if constexpr (sizeof...(Checks) > 0) {
            bool fully_validated = false;

            while (!fully_validated) {
                user_input = input_function();
                std::cout << std::endl;
                
                // Make sure that all checks are successful
                fully_validated = true;
                for (Check check : { checks... }) {
                    try {
                        check(user_input);
                    } catch (const InvalidInputException& input_error) {
                        std::cerr << input_error.what() << std::endl;
                        fully_validated = false;
                        break;
                    }
                }
            }
        } else {
            user_input = input_function();
            std::cout << std::endl;
        }

        return user_input;
    }
    

    template <Style style = Style::Basic, bool prompt_once = false, class... Checks>
    std::string input(std::string message, Checks... checks) {
        /*
        The master input function.

        Template Arguments
        --------------------
            Style       (Style::Basic) | Style | The style of input to use
            Prompt Once (false)        | bool  | Determines if the user is prompted once, or prior to each input call

        Arguments
        --------------------
            Message  | std::string          | The message to prompt the user with prior to taking in any input
            Check... | void(*)(std::string) | A validation predicate to call on the user's input to determine if it's "valid" or not

        Returns
        --------------------
            User Input | std::string | The user's input
        */

        InputFunction input_function;

        // Determine which input function to use based on the selected Style
        switch (style) {
        case Style::Basic:
            input_function = std::bind(infuncs::basic_input, std::nullopt);
            break;
        case Style::Masked:
            input_function = std::bind(infuncs::basic_input, INPUT_MASK);
            break;
        case Style::Instant:
            input_function = infuncs::instant_input;
            break;
        }

        // Prompt once or wrap the input function to have it prompt the user each time
        if (prompt_once) {
            std::cout << message << std::flush;
        } else {
            input_function = [=]() -> std::string {
                std::cout << message << std::flush;
                return input_function();
            };
        }

        return validate(input_function, checks...);
    }

    template <Style style = Style::Basic, bool prompt_once = false, class... Checks>
    std::string get(std::string message, Checks... checks) {
        /*
        The master input function for retrieving information from the user in a computer-like manner.

        Template Arguments
        --------------------
            Style       (Style::Basic) | Style | The style of input to use
            Prompt Once (false)        | bool  | Determines if the user is prompted once, or prior to each input call

        Arguments
        --------------------
            Message  | std::string          | The message to prompt the user with prior to taking in any input
            Check... | void(*)(std::string) | A predicate to call on the user's input to determine if it's "valid" or not

        Returns
        --------------------
            User Input | std::string | The user's input
        */

        message += ": ";
        std::string user_input = input<style, prompt_once>(message, checks...);

        return user_input;
    }

    template <Style style = Style::Basic, bool prompt_once = true, class... Checks>
    std::string ask(std::string question, Checks... checks) {
        /*
        The master input function for retrieving information from the user in a human-like manner.

        Template Arguments
        --------------------
            Style       (Style::Basic) | Style | The style of input to use
            Prompt Once (false)        | bool  | Determines if the user is prompted once, or prior to each input call

        Arguments
        --------------------
            Question    | std::string          | The question to ask the user prior to taking in any input
            Check...    | void(*)(std::string) | A predicate to call on the user's input to determine if it's "valid" or not

        Returns
        --------------------
            User Input | std::string | The user's input
        */

        question += "?\n";
        std::string user_input = input<style, prompt_once>(question, checks...);

        return user_input;
    }

    namespace checks {
        template <bool case_sensitive = false, class... Strings>
        Check is(Strings... strings) {
            /*
            Returns a check that can be called on a user's input to determine if it is equal to any of the given strings.

            Template Arguments
            --------------------
                Case Sensitive (false) | bool | Determines whether or not to compare the strings with case in mind

            Arguments
            --------------------
                String... | std::string | The string to compare the user's input to

            Returns
            --------------------
                Check | void(*)(std::string) | A check to call on the user's input

            Throws
            --------------------
                Invalid Input | InvalidInputException | Thrown when the user's input is not equal to any of the given strings
            */

            return [=](std::string user_input) {
                std::vector<std::string> comparison_strings = { strings... };

                if (!case_sensitive) {
                    for (char& character  : user_input) {
						character = toupper(character);
                    }

                    for (std::string& string : comparison_strings) {
                        for (char& character : string) {
							character = toupper(character);
                        }
                    }
                }

                for (std::string string : comparison_strings) {
                    if (user_input == string) {
                        return;
                    }
                }

                throw InvalidInputException();
            };
        }

        Check matches_regex(std::regex regular_expression) {
            /*
            Returns a check that can be called on a user's input to determine if it consists exclusively of the characters in the given string.

            Arguments
            --------------------
                Regular Expression | std::regex | The regular expression to match the user's input to

            Returns
            --------------------
                Check | void(*)(std::string) | A check to call on the user's input
            */

            return [=](std::string user_input) {
                if (!std::regex_match(user_input, regular_expression)) {
                    throw InvalidInputException();
                }
            };
        }

        Check length(unsigned int length) {
            /*
            Returns a check that can be called on a user's input to determine if it's a certain length.

            Arguments
            --------------------
                Length | unsigned int | The length that the user's input must be

            Returns
            --------------------
                Check | void(*)(std::string) | A check to call on the user's input
            */

            std::regex expression("^.{" + std::to_string(length) + "}$");
            return matches_regex(expression);
        }

        Check consists_of(std::string string_of_char) {
            /*
            Returns a check that can be called on a user's input to determine if it consists exclusively of the characters in the given string.

            Arguments
            --------------------
                String Of Characters | std::string | The string of characters you want to check for 

            Returns
            --------------------
                Check | void(*)(std::string) | A check to call on the user's input
            */

            std::regex expression("^[" + string_of_char + "]+$");
            return matches_regex(expression);
        }

        template <class T = int>
        Check numeric() {
            /*
            Returns a check that can be called on a user's input to determine if it consists exclusively of the characters in the given string.

            Template Arguments
            --------------------
                Numeric Type (int) | T | The numeric type to check for

            Returns
            --------------------
                Check | void(*)(std::string) | A check to call on the user's input

            Throws
            --------------------
                Invalid Argument | std::invalid_argument | Thrown when T cannot be considered a numeric type
            */

            std::string expression = "^";

            // Build the appropriate RegEx
            if (std::is_arithmetic<T>::value) {
                if (std::is_floating_point<T>::value) {
                    expression += "-?[0-9]+\\.[0-9]+";
                } else {
                    if (std::is_signed<T>::value) {
                        expression += "-?";
                    }
                    expression += "[0-9]+";
                }
            } else {
                throw std::invalid_argument("Type T must be numeric");
            }

            expression += '$';

            return matches_regex(std::regex(expression));
        }

        template <class T = int>
        Check range(T mininimum=std::numeric_limits<T>::min(), T maximum=std::numeric_limits<T>::max()) {
            /*
            Returns a check that can be called on a user's input to determine if it's in a given range (INCLUSIVE).

            Template Arguments
            --------------------
                Numeric Type (int) | T | The numeric type to check for
                
            Arguments
            --------------------
                Minimum (std::numeric_limits<T>::min()) | T | The minimum of the range
                Maximum (std::numeric_limits<T>::max()) | T | The maximum of the range

            Returns
            --------------------
                Check | void(*)(std::string) | A check to call on the user's input

            Check Dependencies
            --------------------
                numeric
            */

            return [=](std::string user_input) {
                numeric<T>()(user_input);

                std::stringstream stream(user_input);
                T user_input_numeric;
                stream >> user_input_numeric;

                if (!stream || (user_input_numeric < mininimum || user_input_numeric > maximum)) {
                    throw InvalidInputException();
                }
            };
        }

        namespace wrappers {
            Check custom(Check check, std::string error_message) {
                /*
                Gives any given check a new error message.

                Arguments
                --------------------
                    Check         | void(*)(std::string) | The check to attach a new error message to 
                    Error Message | std::string          | The new error message for the check
                    
                Returns
                --------------------
                    Check | void(*)(std::string) | A check to call on the user's input
                */

                return [=](std::string user_input) {
                    try {
                        check(user_input);
                    } catch (const InvalidInputException& exception) {
                        throw InvalidInputException(error_message.c_str());
                    }
                };
            }

            Check inverse(Check check) {
                /*
                Returns the inverse of a given check.

                Arguments
                --------------------
                    Check | void(*)(std::string) | The check to attach a new error message to 

                Returns
                --------------------
                    Check | void(*)(std::string) | A check to call on the user's input
                */

                return [=](std::string user_input) {
                    try {
                        check(user_input);
                    } catch (const InvalidInputException& exception) {
                        return;
                    }

                    throw InvalidInputException();
                };
            }

            template <class... Checks>
            Check any(Checks... checks) {
                /*
                Creates a check that passes if any one of the given checks pass

                Arguments
                --------------------
                    Check...    | void(*)(std::string) | The checks to call on the user's input
                    
                Returns
                --------------------
                    Check | void(*)(std::string) | A check to call on the user's input
                */

                return [=](std::string user_input) {
                    for (Check check : { checks... }) {
                        try {
                            check(user_input);
                        } catch (const InvalidInputException& exception) {
                            continue;
                        }

                        return;
                    }

                    throw InvalidInputException();
                };
            }
        }
    }
}

#endif
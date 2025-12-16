

/*
 * ProjectTwo.cpp
 * ABCU CS Advising Assistance Program
 *
 * Data structure: Self-balancing BST (AVL) keyed by course number, as recommended in Project One.
 * Features:
 *  - Load course data from a user-provided filename (CSV or flexible delimited)
 *  - Store courses in AVL tree (alphanumeric order by course number)
 *  - Print all courses in-order (sorted)
 *  - Print individual course info (title + prerequisites with titles if known)
 *  - Robust input validation and clear error messages
 *
 * Compile:
 *   g++ -std=c++17 -O2 -Wall -Wextra -pedantic ProjectTwo.cpp -o advising
 *
 * Run:
 *   ./advising
 *
 * Notes:
 *  - CSV columns expected: courseNumber, title, prereq1, prereq2 (empty allowed).
 *  - Lines starting with '#' or blank lines are ignored.
 *  - Course/prereq codes are normalized to uppercase for consistent lookup.
 */

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// -------------------------- Data Model --------------------------

struct Course {
    std::string number;                     // e.g., "CSCI200"
    std::string title;                      // e.g., "Data Structures"
    std::vector<std::string> prerequisites; // e.g., {"CSCI100", "MATH201"}
};

// -------------------------- String Utilities --------------------------

static std::string trim(const std::string& s) {
    size_t start = 0, end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

static std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

static std::string toUpper(std::string s) {
    for (char& ch : s) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return s;
}

// Lightweight CSV splitter: supports quoted fields and commas
static std::vector<std::string> splitCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && (i + 1) < line.size() && line[i + 1] == '"') {
                field.push_back('"'); // Escaped quote within quoted field
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field.push_back(c);
        }
    }
    fields.push_back(field);
    return fields;
}

// Split prerequisites by multiple delimiters for flexibility (| ; , whitespace)
static std::vector<std::string> splitPrereqTokens(const std::string& s) {
    std::vector<std::string> tokens;
    std::string token;
    auto flush = & {
        std::string t = trim(token);
        if (!t.empty()) tokens.push_back(t);
        token.clear();
    };

    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c)) || c == '|' || c == ';' || c == ',') {
            flush();
        } else {
            token.push_back(c);
        }
    }
    flush();
    return tokens;
}

// -------------------------- AVL Tree (Self-balancing BST) --------------------------

struct AVLNode {
    std::string key; // course number
    Course value;
    int height;
    AVLNode* left;
    AVLNode* right;

    AVLNode(const std::string& k, const Course& v)
        : key(k), value(v), height(1), left(nullptr), right(nullptr) {}
};

static int nodeHeight(AVLNode* n) { return n ? n->height : 0; }

static int balanceFactor(AVLNode* n) { return n ? nodeHeight(n->left) - nodeHeight(n->right) : 0; }

static void updateHeight(AVLNode* n) {
    if (n) n->height = 1 + std::max(nodeHeight(n->left), nodeHeight(n->right));
}

static AVLNode* rotateRight(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;
    x->right = y;
    y->left = T2;
    updateHeight(y);
    updateHeight(x);
    return x;
}

static AVLNode* rotateLeft(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    updateHeight(x);
    updateHeight(y);
    return y;
}

static AVLNode* avlInsert(AVLNode* node, const std::string& key, const Course& value) {
    if (!node) return new AVLNode(key, value);

    if (key < node->key) {
        node->left = avlInsert(node->left, key, value);
    } else if (key > node->key) {
        node->right = avlInsert(node->right, key, value);
    } else {
        // Duplicate key: overwrite value (latest wins)
        node->value = value;
        return node;
    }

    updateHeight(node);
    int bf = balanceFactor(node);

    // Left Left
    if (bf > 1 && key < node->left->key) return rotateRight(node);
    // Right Right
    if (bf < -1 && key > node->right->key) return rotateLeft(node);
    // Left Right
    if (bf > 1 && key > node->left->key) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    // Right Left
    if (bf < -1 && key < node->right->key) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

static AVLNode* avlFind(AVLNode* node, const std::string& key) {
    while (node) {
        if (key < node->key) node = node->left;
        else if (key > node->key) node = node->right;
        else return node;
    }
    return nullptr;
}

static void avlInOrder(AVLNode* node) {
    if (!node) return;
    avlInOrder(node->left);
    std::cout << node->key << ": " << node->value.title << "\n";
    avlInOrder(node->right);
}

static void avlDestroy(AVLNode* node) {
    if (!node) return;
    avlDestroy(node->left);
    avlDestroy(node->right);
    delete node;
}

// -------------------------- Loading & Parsing --------------------------

/**
 * Parse one line into a Course.
 * Expects fields:
 *   [0] course number
 *   [1] title
 *   [2..] prerequisites (may be multiple fields, possibly empty; can also contain combined tokens like "CSCI200 | MATH201")
 */
static bool parseCourseLine(const std::string& rawLine, Course& outCourse, std::string& errMsg) {
    std::string line = rawLine;
    if (!line.empty() && line.back() == '\r') line.pop_back(); // Handle CRLF

    // Skip if blank or comment
    std::string t = trim(line);
    if (t.empty() || t[0] == '#') {
        errMsg = "skip"; // marker for caller
        return false;
    }

    // CSV split (quoted supported)
    auto fields = splitCSVLine(line);

    // Normalize: trim & strip quotes
    for (std::string& f : fields) {
        f = stripQuotes(trim(f));
    }

    if (fields.size() < 2 || fields[0].empty() || fields[1].empty()) {
        errMsg = "Malformed line: requires course number and title.";
        return false;
    }

    outCourse.number = toUpper(fields[0]);
    outCourse.title  = fields[1];

    std::vector<std::string> prereqs;
    for (size_t i = 2; i < fields.size(); ++i) {
        if (!fields[i].empty()) {
            auto tokens = splitPrereqTokens(fields[i]); // allows "CSCI200 | MATH201"
            for (auto& tok : tokens) {
                if (!tok.empty()) prereqs.push_back(toUpper(tok));
            }
        }
    }

    // Deduplicate prerequisites
    std::sort(prereqs.begin(), prereqs.end());
    prereqs.erase(std::unique(prereqs.begin(), prereqs.end()), prereqs.end());
    outCourse.prerequisites = std::move(prereqs);

    return true;
}

/**
 * Load courses from file into AVL tree (by inserting each parsed Course).
 * Returns true if at least one valid course was loaded.
 */
static bool loadCoursesFromFile(const std::string& filename, AVLNode*& root) {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "ERROR: Could not open file '" << filename << "'. Check the path and try again.\n";
        return false;
    }

    root = nullptr; // reset tree
    std::string line;
    size_t lineNumber = 0, added = 0, skipped = 0;

    while (std::getline(in, line)) {
        ++lineNumber;

        Course c;
        std::string err;
        bool ok = parseCourseLine(line, c, err);
        if (!ok) {
            if (err == "skip") continue; // blank/comment line
            std::cerr << "WARN (line " << lineNumber << "): " << err << "\n";
            ++skipped;
            continue;
        }

        root = avlInsert(root, c.number, c);
        ++added;
    }

    std::cout << "Loaded " << added << " courses";
    if (skipped > 0) std::cout << " (" << skipped << " skipped due to errors)";
    std::cout << " from '" << filename << "'.\n";

    if (added == 0) {
        std::cerr << "ERROR: No valid course records were loaded. Verify file format.\n";
        return false;
    }
    return true;
}

// -------------------------- Printing --------------------------

static void printAllCourses(AVLNode* root) {
    if (!root) {
        std::cout << "No courses loaded. Use Option 1 to load data first.\n";
        return;
    }
    std::cout << "---- Computer Science Course List (Alphanumeric) ----\n";
    avlInOrder(root);
    std::cout << "-----------------------------------------------------\n";
}

static void printCourseInfo(AVLNode* root, const std::string& courseNumberRaw) {
    if (!root) {
        std::cout << "No courses loaded. Use Option 1 to load data first.\n";
        return;
    }

    std::string key = toUpper(trim(courseNumberRaw));
    AVLNode* node = avlFind(root, key);
    if (!node) {
        std::cout << "Course '" << key << "' was not found. Please check the course number and try again.\n";
        return;
    }

    const Course& c = node->value;
    std::cout << "Course: " << c.number << " - " << c.title << "\n";
    if (c.prerequisites.empty()) {
        std::cout << "Prerequisites: None\n";
    } else {
        std::cout << "Prerequisites:\n";
        for (const std::string& p : c.prerequisites) {
            AVLNode* pn = avlFind(root, p);
            if (pn) {
                std::cout << "  - " << p << " - " << pn->value.title << "\n";
            } else {
                std::cout << "  - " << p << " - (title unknown)\n";
            }
        }
    }
}

// -------------------------- Menu --------------------------

static void printMenu() {
    std::cout << "\n"
              << "================ Advising Assistance Menu ================\n"
              << "  1. Load file data into the data structure\n"
              << "  2. Print an alphanumeric list of all courses\n"
              << "  3. Print course information (title and prerequisites)\n"
              << "  9. Exit the program\n"
              << "==========================================================\n"
              << "Enter your choice: ";
}

int main() {
    AVLNode* root = nullptr;
    bool dataLoaded = false;

    while (true) {
        printMenu();

        std::string choiceLine;
        if (!std::getline(std::cin, choiceLine)) {
            std::cerr << "\nERROR: Input stream closed unexpectedly. Exiting.\n";
            break;
        }
        std::string choiceTrim = trim(choiceLine);
        if (choiceTrim.empty()) {
            std::cout << "Please enter a valid option number.\n";
            continue;
        }

        int choice = -1;
        try { choice = std::stoi(choiceTrim); }
        catch (...) { std::cout << "Invalid input. Please enter 1, 2, 3, or 9.\n"; continue; }

        if (choice == 9) {
            std::cout << "Exiting Advising Assistance Program. Goodbye!\n";
            break;
        }

        switch (choice) {
            case 1: {
                std::cout << "Enter the filename containing course data (e.g., CS 300 ABCU_Advising_Program_Input.csv): ";
                std::string filename;
                if (!std::getline(std::cin, filename)) {
                    std::cerr << "ERROR: Failed to read filename.\n";
                    continue;
                }
                filename = trim(filename);
                if (filename.empty()) {
                    std::cout << "Filename cannot be empty.\n";
                    continue;
                }

                if (loadCoursesFromFile(filename, root)) {
                    dataLoaded = true;
                } else {
                    // keep root as nullptr if failed
                    avlDestroy(root);
                    root = nullptr;
                    dataLoaded = false;
                }
                break;
            }

            case 2: {
                if (!dataLoaded) {
                    std::cout << "Please load data (Option 1) before printing the course list.\n";
                    break;
                }
                printAllCourses(root);
                break;
            }

            case 3: {
                if (!dataLoaded) {
                    std::cout << "Please load data (Option 1) before printing course information.\n";
                    break;
                }
                std::cout << "Enter the course number to look up (e.g., CSCI300): ";
                std::string courseNumber;
                if (!std::getline(std::cin, courseNumber)) {
                    std::cerr << "ERROR: Failed to read course number.\n";
                    continue;
                }
                if (trim(courseNumber).empty()) {
                    std::cout << "Course number cannot be empty.\n";
                    continue;
                }
                printCourseInfo(root, courseNumber);
                break;
            }

            default:
                std::cout << "Unknown option. Please enter 1, 2, 3, or 9.\n";
                break;
        }
    }

    avlDestroy(root);
    return 0;
}
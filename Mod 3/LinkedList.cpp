//============================================================================
// Name        : LinkedList.cpp
// Author      : Jacob Griggs
// Version     : 1.0
// Copyright   : Copyright � 2023 SNHU COCE
// Description : Lab 3-2 Lists and Searching
//============================================================================

#include <algorithm>
#include <iostream>
#include <time.h>

#include "CSVparser.hpp"

using namespace std;

//============================================================================
// Global definitions visible to all methods and classes
//============================================================================

// forward declarations
double strToDouble(string str, char ch);

// define a structure to hold bid information
struct Bid {
    string bidId; // unique identifier
    string title;
    string fund;
    double amount;
    Bid() {
        amount = 0.0;
    }
};

//============================================================================
// Linked-List class definition
//============================================================================

/**
 * Define a class containing data members and methods to
 * implement a linked-list.
 */
class LinkedList {

private:
    //Internal structure for list entries, housekeeping variables
    struct Node {
        Bid bid;
        Node *next;

        // default constructor
        Node() {
            next = nullptr;
        }

        // initialize with a bid
        Node(Bid aBid) {
            bid = aBid;
            next = nullptr;
        }
    };

    Node* head;
    Node* tail;
    int size = 0;

public:
    LinkedList();
    virtual ~LinkedList();
    void Append(Bid bid);
    void Prepend(Bid bid);
    void PrintList();
    void Remove(string bidId);
    Bid Search(string bidId);
    int Size();
};

/**
 * Default constructor
 */
LinkedList::LinkedList() {
   head = nullptr;
   tail = nullptr;
}

/**
 * Destructor
 */
LinkedList::~LinkedList() {
    Node* current = head;
    Node* temp;
    while (current != nullptr) { // loop over each node, detach from list then delete
        temp = current; // hang on to current node
        current = current->next; // make current the next node
        delete temp; // delete the orphan node
    }
    
 head = nullptr;  // list is now empty
 tail = nullptr;

}

/**
 * Append a new bid to the end of the list
 */


/**
 * Prepend a new bid to the start of the list
 */
void LinkedList::Prepend(Bid bid) {
     Node* newNode = new Node(bid);// Create new node

    if (head != nullptr) {// if there is already something at the head...
    
        newNode->next = head; // new node points to current head as its next node
        head = newNode; // head now becomes the new node
      }else {
        head = newNode; // head is equal to new node
        tail = newNode; // tail is equal to new node
      }
    size++; //increase size count
}


/**
 * Simple output of all bids in the list
 */
void LinkedList::PrintList() {
     Node* current = head;// start at the head
    while (current != nullptr) {// while loop over each node looking for a match
        cout << current->bid.bidId << ": " 
        << current->bid.title << " | "
        << current->bid.amount << " | " 
        << current->bid.fund << endl;//output current bidID, title, amount and fund
        current = current->next;//set current equal to next
}
}

/**
 * Remove a specified bid
 *
 * @param bidId The bid id to remove from the list
 */


void LinkedList::Remove(string bidId) {
    // Special case: head matches
    if (head != nullptr && head->bid.bidId == bidId) {
        Node* temp = head;
        head = head->next; // move head to next node
        if (head == nullptr) {
            tail = nullptr; // list is now empty
        }
        delete temp; // free memory
        size--; // decrease size count
        return;
    }

    // Traverse the list
    Node* current = head;
    while (current != nullptr && current->next != nullptr) {
        if (current->next->bid.bidId == bidId) {
            Node* temp = current->next; // hold onto the next node
            current->next = temp->next; // bypass the node
            if (temp == tail) {
                tail = current; // update tail if last node removed
            }
            delete temp; // free memory
            size--; // decrease size count
            return;
        }
        current = current->next; // move to next node
    }
}



/**
 * Search for the specified bidId
 *
 * @param bidId The bid id to search for
 */
Bid LinkedList::Search(string bidId) {
    // special case if matching bid is the head
    Node*current = head;// start at the head of the list
    while(current != nullptr) { // keep searching until end reached with while loop (current != nullptr)
        if(current->bid.bidId == bidId) {   // if current node bidID is equal to search bidID
            return current->bid; // return the current bid
        } else {
            current = current->next; // else current node is equal to next node
        }
    }
    //(the next two statements will only execute if search item is not found)
       Bid emptyBid; //create new empty bid
        return emptyBid;// return the empty bid 
}

/**
 * Returns the current size (number of elements) in the list
 */
int LinkedList::Size() {
    return size;
}

//============================================================================
// Static methods used for testing
//============================================================================

/**
 * Display the bid information
 *
 * @param bid struct containing the bid info
 */
void displayBid(Bid bid) {
    cout << bid.bidId << ": " << bid.title << " | " << bid.amount
         << " | " << bid.fund << endl;
    return;
}

/**
 * Prompt user for bid information
 *
 * @return Bid struct containing the bid info
 */
Bid getBid() {
    Bid bid;

    cout << "Enter Id: ";
    cin.ignore();
    getline(cin, bid.bidId);

    cout << "Enter title: ";
    getline(cin, bid.title);

    cout << "Enter fund: ";
    cin >> bid.fund;

    cout << "Enter amount: ";
    cin.ignore();
    string strAmount;
    getline(cin, strAmount);
    bid.amount = strToDouble(strAmount, '$');

    return bid;
}

/**
 * Load a CSV file containing bids into a LinkedList
 *
 * @return a LinkedList containing all the bids read
 */
void loadBids(string csvPath, LinkedList *list) {
    cout << "Loading CSV file " << csvPath << endl;

    // initialize the CSV Parser
    csv::Parser file = csv::Parser(csvPath);

    try {
        // loop to read rows of a CSV file
        for (int i = 0; i < file.rowCount(); i++) {

            // initialize a bid using data from current row (i)
            Bid bid;
            bid.bidId = file[i][1];
            bid.title = file[i][0];
            bid.fund = file[i][8];
            bid.amount = strToDouble(file[i][4], '$');

            //cout << bid.bidId << ": " << bid.title << " | " << bid.fund << " | " << bid.amount << endl;

            // add this bid to the end
            list->Append(bid);
        }
    } catch (csv::Error &e) {
        std::cerr << e.what() << std::endl;
    }
}

/**
 * Simple C function to convert a string to a double
 * after stripping out unwanted char
 *
 * credit: http://stackoverflow.com/a/24875936
 *
 * @param ch The character to strip out
 */
double strToDouble(string str, char ch) {
    str.erase(remove(str.begin(), str.end(), ch), str.end());
    return atof(str.c_str());
}

/**
 * The one and only main() method
 *
 * @param arg[1] path to CSV file to load from (optional)
 * @param arg[2] the bid Id to use when searching the list (optional)
 */
int main(int argc, char* argv[]) {

    // process command line arguments
    string csvPath, bidKey;
    switch (argc) {
    case 2:
        csvPath = argv[1];
        bidKey = "98109";
        break;
    case 3:
        csvPath = argv[1];
        bidKey = argv[2];
        break;
    default:
        csvPath = "eBid_Monthly_Sales.csv";
        bidKey = "98109";
    }

    clock_t ticks;

    LinkedList bidList;

    Bid bid;

    int choice = 0;
    while (choice != 9) {
        cout << "Menu:" << endl;
        cout << "  1. Enter a Bid" << endl;
        cout << "  2. Load Bids" << endl;
        cout << "  3. Display All Bids" << endl;
        cout << "  4. Find Bid" << endl;
        cout << "  5. Remove Bid" << endl;
        cout << "  9. Exit" << endl;
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice) {
        case 1:
            bid = getBid();
            bidList.Append(bid);
            displayBid(bid);

            break;

        case 2:
            ticks = clock();

            loadBids(csvPath, &bidList);

            cout << bidList.Size() << " bids read" << endl;

            ticks = clock() - ticks; // current clock ticks minus starting clock ticks
            cout << "time: " << ticks << " milliseconds" << endl;
            cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;

            break;

        case 3:
            bidList.PrintList();

            break;

        case 4:
            ticks = clock();

            bid = bidList.Search(bidKey);

            ticks = clock() - ticks; // current clock ticks minus starting clock ticks

            if (!bid.bidId.empty()) {
                displayBid(bid);
            } else {
            	cout << "Bid Id " << bidKey << " not found." << endl;
            }

            cout << "time: " << ticks << " clock ticks" << endl;
            cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;

            break;

        case 5:
            bidList.Remove(bidKey);

            break;
        }
    }

    cout << "Good bye." << endl;

    return 0;
}

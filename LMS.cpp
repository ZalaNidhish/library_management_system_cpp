#include <iostream>
#include <fstream>
#include<string>
#include <iomanip>
#include <cerrno>
#include <cstdlib>
#include <stdexcept>
#include <ctime>
using namespace std;

#define RESET   "\033[0m"
#define CYAN    "\033[96m"
#define BLUE    "\033[94m"
#define GREEN   "\033[92m"
#define YELLOW  "\033[93m"
#define RED     "\033[91m"
#define WHITE   "\033[97m"
#define GRAY    "\033[90m"
#define BOLD    "\033[1m"

void registerUser();
string loginUser(int &);
void listAllUsers();
void addBook();
void listBooks();
void updateBook();
void deleteBook();
void issueBook(int);
void returnBook(int);
void listIssuedBooks(int);
void addUserToFile(int, string, string, string, string);

string trim(string s) {
    size_t start = s.find_first_not_of(" \t\n\r");

    if (start == string::npos)
        return "";

    size_t end = s.find_last_not_of(" \t\n\r");

    return s.substr(start, end - start + 1);
}

// ---------- CLI helpers ----------

void printLine(char c = '-') {
    cout << GRAY << string(103, c) << RESET << "\n";
}

void printHeader(string title) {
    int pad = (103 - title.size()) / 2;
    printLine('=');
    cout << string(pad, ' ') << BOLD << CYAN << title << RESET << "\n";
    printLine('=');
}

void waitEnter() {
    string dummy;
    cout << GRAY << "\nPress Enter to continue..." << RESET;
    getline(cin, dummy);
}

// Reads a whole number safely (no crash on letters)
int readInt(string msg) {
    string s;
    while (true) {
        cout << msg;
        if (!getline(cin, s)) exit(0);          // input closed (Ctrl+D)
        try {
            return stoi(s);
        } catch (...) {
            cout << RED << "Invalid number, try again.\n" << RESET;
        }
    }
}

// today's date as DD-MM-YYYY
string today() {
    time_t t = time(0);
    char buf[20];
    strftime(buf, sizeof(buf), "%d-%m-%Y", localtime(&t));
    return buf;
}

// ---------- File helpers (headers + rows) ----------

bool isFileEmpty(string filename) {
    ifstream f(filename);
    return !f || f.peek() == ifstream::traits_type::eof();
}

void writeUserHeader(ostream &out) {
    out << left << setw(5)  << "ID"       << setw(3) << "|"
        << setw(15) << "Name"             << setw(3) << "|"
        << setw(20) << "Email"            << setw(3) << "|"
        << setw(15) << "Password"         << setw(3) << "|"
        << setw(15) << "Role"             << setw(3) << "|" << '\n';
}

void writeBookHeader(ostream &out) {
    out << left << setw(8)  << "ID"       << setw(3) << "|"
        << setw(30) << "Title"            << setw(3) << "|"
        << setw(25) << "Author"           << setw(3) << "|"
        << setw(20) << "Publication"      << setw(3) << "|"
        << setw(8)  << "Quantity"         << setw(3) << "|" << '\n';
}

void writeBookRow(ostream &out, string id, string title, string author, string publication, string qty) {
    out << left
        << setw(8)  << id
        << setw(3)  << "|"
        << setw(30) << title
        << setw(3)  << "|"
        << setw(25) << author
        << setw(3)  << "|"
        << setw(20) << publication
        << setw(3)  << "|"
        << setw(8)  << qty
        << setw(3)  << "|" << '\n';
}

void writeIssuedHeader(ostream &out) {
    out << left << setw(8)  << "Book ID"    << setw(3) << "|"
        << setw(8)  << "User ID"            << setw(3) << "|"
        << setw(12) << "Issue Date"         << setw(3) << "|"
        << setw(8)  << "Issued"             << setw(3) << "|"
        << setw(10) << "Stock Left"         << setw(3) << "|" << '\n';
}

void writeIssuedRow(ostream &out, string bookID, string userID, string date, string qty, string stock) {
    out << left
        << setw(8)  << bookID
        << setw(3)  << "|"
        << setw(8)  << userID
        << setw(3)  << "|"
        << setw(12) << date
        << setw(3)  << "|"
        << setw(8)  << qty
        << setw(3)  << "|"
        << setw(10) << stock
        << setw(3)  << "|" << '\n';
}

// ---------- Auto ID generation ----------

// checks whether an ID is already used in a file (hasHeader = skip first line)
bool idExists(string filename, int id, bool hasHeader) {
    ifstream in(filename);
    string row;

    if (!in) return false;                      
    if (hasHeader) getline(in, row);

    while (getline(in, row)) {
        size_t bar = row.find('|');
        if (bar == string::npos) continue;
        if (trim(row.substr(0, bar)) == to_string(id)) return true;
    }
    return false;
}

// returns a random, unused 3 digit ID (100 - 999)
int generateID(string filename) {
    for (int tries = 0; tries < 5000; tries++) {
        int id = 100 + rand() % 900;
        if (!idExists(filename, id, true)) return id;
    }
    throw runtime_error("No free 3 digit IDs left in " + filename);
}

// ---------- Classes ----------

class User{
    int id;
    string name;
    string email;
    string password;
    string role;

public:
    User(int id, string name, string email, string password, string role="user"){
        this->id = id;
        this->name=name;
        this->email=email;
        this->password=password;
        this->role=role;
    }

    void display(){
        cout<<"\n\t----- User Information -----\t\n";
        cout<<"User ID: "<<id<<endl;
        cout<<"User name: "<<name<<endl;
        cout<<"User email: "<<email<<endl;
        cout<<"User password: "<<password<<endl;
    }

    string getRole(){return role;}
    int getUserID(){return id;}

};

class Book{
    int id;
    int qty;
    string title;
    string author;
    string publication;

public:
    Book(int id, int qty, string title, string author, string publication){
        this->id=id;
        this->qty=qty;
        this->title=title;
        this->author=author;
        this->publication=publication;

        bool needHeader = isFileEmpty("books.txt");

        ofstream out("books.txt", ios::app);

        if (!out) {
            cout << "File opening failed!\n";
            return;
        }

        if (needHeader) writeBookHeader(out);

        writeBookRow(out, to_string(id), title, author, publication, to_string(qty));

        out.close();
        out.clear();

        cout << GREEN << "Book added successfully!\n" << RESET;
    }
};

// ---------- Stock / issued file functions ----------

int changeStock(int bookID, int change) {
    ifstream in("books.txt");
    ofstream out("temp.txt");

    if (!in || !out) {
        throw runtime_error("books file could not be opened");
    }

    writeBookHeader(out);

    int result = -1;
    string id, qty, title, author, publication, line;

    getline(in, line);                          // skip header

    while (getline(in, id, '|') && getline(in, title, '|') && getline(in, author, '|') && getline(in, publication, '|') && getline(in, qty, '|')) {

        id = trim(id);
        title = trim(title);
        author = trim(author);
        publication = trim(publication);
        qty = trim(qty);

        if (id.empty()) {
            continue;
        }

        if (stoi(id) == bookID) {
            int newQty = stoi(qty) + change;
            if (newQty < 0) {
                result = -2;
            } else {
                result = newQty;
                qty = to_string(newQty);
            }
        }

        writeBookRow(out, id, title, author, publication, qty);
    }

    in.close();
    out.close();

    if (result < 0) {                           // nothing to save
        remove("temp.txt");
        return result;
    }

    remove("books.txt");
    rename("temp.txt", "books.txt");
    return result;
}

int getIssuedQty(int bookID, int userID) {
    ifstream in("issued_books.txt");
    int total = 0;

    if (!in) return 0;

    string b, u, d, q, s, line;
    getline(in, line);                          // skip header

    while (getline(in, b, '|') && getline(in, u, '|') && getline(in, d, '|') && getline(in, q, '|') && getline(in, s, '|')) {
        b = trim(b);
        u = trim(u);
        q = trim(q);

        if (b.empty()) continue;

        if (stoi(b) == bookID && (userID == 0 || stoi(u) == userID)) {
            total = total + stoi(q);
        }
    }

    in.close();
    return total;
}

void updateIssued(int bookID, int userID, int change, int stockLeft) {
    ifstream in("issued_books.txt");
    ofstream out("temp.txt");

    if (!out) {
        throw runtime_error("temp.txt could not be opened");
    }

    writeIssuedHeader(out);

    bool found = false;
    string b, u, d, q, s, line;

    if (in) {
        getline(in, line);                      // skip header

        while (getline(in, b, '|') && getline(in, u, '|') && getline(in, d, '|') && getline(in, q, '|') && getline(in, s, '|')) {
            b = trim(b);
            u = trim(u);
            d = trim(d);
            q = trim(q);
            s = trim(s);

            if (b.empty()) continue;

            if (stoi(b) == bookID) {
                s = to_string(stockLeft);

                if (stoi(u) == userID) {
                    found = true;
                    int newQty = stoi(q) + change;
                    if (change > 0) d = today();
                    if (newQty <= 0) continue;  // everything returned, drop the entry
                    q = to_string(newQty);
                }
            }

            writeIssuedRow(out, b, u, d, q, s);
        }
    }

    if (!found && change > 0) {
        writeIssuedRow(out, to_string(bookID), to_string(userID), today(), to_string(change), to_string(stockLeft));
    }

    in.close();
    out.close();

    remove("issued_books.txt");
    rename("temp.txt", "issued_books.txt");
}

// ---------- Book functions ----------

void addBook() {
    int id, qty;
    string title, author, publication;

    printHeader("ADD BOOK");

    id = generateID("books.txt");
    cout << "Book ID (auto-generated): " << CYAN << id << RESET << endl;

    cout<<"Enter Book Title: ";
    getline(cin, title);

    cout<<"Enter Author Name: ";
    getline(cin, author);

    cout<<"Enter Publication: ";
    getline(cin, publication);

    qty = readInt("Enter Quantity: ");

    Book b(id, qty, title, author, publication);

}

void deleteBook() {

    int targetID;
    bool found = false;

    printHeader("DELETE BOOK");

    targetID = readInt("Enter Book ID to delete: ");

    if (getIssuedQty(targetID, 0) > 0) {
        cout << RED << "Cannot delete: copies of this book are currently issued.\n" << RESET;
        return;
    }

    ifstream in("books.txt");
    ofstream out("temp.txt");

    if (!in || !out) {
        cout << RED << "File opening failed!\n" << RESET;
        return;
    }

    writeBookHeader(out);

    string id, qty, title, author, publication, line;

    getline(in, line);                          // skip header

    while (getline(in, id, '|') && getline(in, title, '|') && getline(in, author, '|') && getline(in, publication, '|') && getline(in, qty, '|')) {

        id = trim(id);
        title = trim(title);
        author = trim(author);
        publication = trim(publication);
        qty = trim(qty);

        if (id.empty()) {
            continue;
        }

        if(stoi(id) == targetID){
            found = true;
            continue;
        }

        writeBookRow(out, id, title, author, publication, qty);
    }

    in.close();
    in.clear();
    out.close();
    out.clear();

    remove("books.txt");
    rename("temp.txt", "books.txt");

    if (found) cout << GREEN << "Deletion successful\n" << RESET;
    else       cout << RED << "No book found with that ID.\n" << RESET;
}

void updateBook() {

    listBooks();

    int targetID;
    bool found = false;
    string newTitle, newAuthor, newPublication, newQty;

    targetID = readInt("\nEnter Book ID to update: ");

    cout << "Enter New Title (Press Enter to keep unchanged): ";
    getline(cin, newTitle);

    cout << "Enter New Author (Press Enter to keep unchanged): ";
    getline(cin, newAuthor);

    cout << "Enter New Publication (Press Enter to keep unchanged): ";
    getline(cin, newPublication);

    cout << "Enter New Quantity (Press Enter to keep unchanged): ";
    getline(cin, newQty);

    // quantity must be a number if entered
    try {
        if (!newQty.empty()) stoi(newQty);
    } catch (...) {
        cout << RED << "Quantity must be a number. Update cancelled.\n" << RESET;
        return;
    }

    ifstream in("books.txt");
    ofstream out("temp.txt");

    if (!in || !out) {
        cout << RED << "File opening failed!\n" << RESET;
        return;
    }

    writeBookHeader(out);

    string id, qty, title, author, publication, line;

    getline(in, line);                          // skip header

    while (getline(in, id, '|') && getline(in, title, '|') && getline(in, author, '|') && getline(in, publication, '|') && getline(in, qty, '|')) {

        id = trim(id);
        title = trim(title);
        author = trim(author);
        publication = trim(publication);
        qty = trim(qty);

        if (id.empty()) {
            continue;
        }

        if(stoi(id) == targetID){
            found = true;
            writeBookRow(out, id,
                         (newTitle.empty() ? title : newTitle),
                         (newAuthor.empty() ? author : newAuthor),
                         (newPublication.empty() ? publication : newPublication),
                         (newQty.empty() ? qty : newQty));
        }else{
            writeBookRow(out, id, title, author, publication, qty);
        }
    }

    in.close();
    in.clear();
    out.close();
    out.clear();

    remove("books.txt");
    rename("temp.txt", "books.txt");

    if (found) cout << GREEN << "Updation successful\n" << RESET;
    else       cout << RED << "No book found with that ID.\n" << RESET;
}

void listBooks() {

    ifstream in("books.txt");
    if (!in) {
        cout << YELLOW << "No books found.\n" << RESET;
        return;
    }
    string id, qty, title, author, publication, line;

    printHeader("ALL BOOK DETAILS");

    cout << BOLD;
    writeBookHeader(cout);
    cout << RESET;

    printLine('-');

    getline(in, line);                          // skip header

    while (getline(in, id, '|') && getline(in, title, '|') && getline(in, author, '|') && getline(in, publication, '|') && getline(in, qty, '|')) {

        id = trim(id);
        title = trim(title);
        author = trim(author);
        publication = trim(publication);
        qty = trim(qty);

        writeBookRow(cout, id, title, author, publication, qty);
    }

    printLine('-');
    in.close();
}

// ---------- Issue / Return ----------

void listIssuedBooks(int userID) {

    ifstream in("issued_books.txt");
    string b, u, d, q, s, line;
    int count = 0;

    printHeader(userID == 0 ? "ALL ISSUED BOOKS" : "MY ISSUED BOOKS");

    cout << BOLD;
    writeIssuedHeader(cout);
    cout << RESET;

    printLine('-');

    if (in) {
        getline(in, line);                      // skip header

        while (getline(in, b, '|') && getline(in, u, '|') && getline(in, d, '|') && getline(in, q, '|') && getline(in, s, '|')) {
            b = trim(b);
            u = trim(u);
            d = trim(d);
            q = trim(q);
            s = trim(s);

            if (b.empty()) continue;

            if (userID == 0 || stoi(u) == userID) {
                writeIssuedRow(cout, b, u, d, q, s);
                count++;
            }
        }
    }

    if (count == 0) {
        cout << YELLOW << "No issued books.\n" << RESET;
    }

    printLine('-');
    in.close();
}

void issueBook(int userID) {

    listBooks();

    int bookID = readInt("\nEnter ID of the Book to issue: ");
    int qty = readInt("Enter number of copies: ");

    if (qty <= 0) {
        cout << RED << "Number of copies must be at least 1.\n" << RESET;
        return;
    }

    int left = changeStock(bookID, -qty);

    if (left == -1) {
        cout << RED << "No book found with that ID.\n" << RESET;
        return;
    }
    if (left == -2) {
        cout << RED << "Not enough copies available.\n" << RESET;
        return;
    }

    updateIssued(bookID, userID, qty, left);
    cout << GREEN << "Book issued successfully! Copies left in library: " << left << RESET << endl;
}

void returnBook(int userID) {

    listIssuedBooks(userID);

    int bookID = readInt("\nEnter ID of the Book to return: ");
    int issued = getIssuedQty(bookID, userID);

    if (issued == 0) {
        cout << RED << "You have not issued this book, so it cannot be returned.\n" << RESET;
        return;
    }

    int qty = readInt("Enter number of copies to return: ");

    if (qty <= 0 || qty > issued) {
        cout << RED << "You can return between 1 and " << issued << " copies.\n" << RESET;
        return;
    }

    int left = changeStock(bookID, qty);

    if (left < 0) {
        cout << RED << "Book record is missing in the library.\n" << RESET;
        return;
    }

    updateIssued(bookID, userID, -qty, left);
    cout << GREEN << "Book returned successfully! Copies now in library: " << left << RESET << endl;
}

// ---------- User functions ----------

void registerUser(){
    int id;
    string name, email, password, cpass;

    printHeader("USER REGISTRATION");

    id = generateID("users.txt");
    cout << "Your User ID (auto-generated): " << CYAN << id << RESET << endl;

    cout << "Enter your name: ";
    getline(cin, name);
    cout << "Enter your email: ";
    getline(cin, email);
    cout << "Enter your password: ";
    getline(cin, password);
    cout << "Confirm password: ";
    getline(cin, cpass);
    if(password != cpass){
        cout << RED << "Password Doesnt match\n" << RESET;
        return;
    }

    User u(id, name, email, password);
    addUserToFile(id, name, email, password, "user");
    cout << GREEN << "User Registered Successfully\n" << RESET;
    u.display();
}

string loginUser(int &userID){
    string uemail, upassword, email, password, line, id, name, role;
    bool flag = false;

    printHeader("USER LOGIN");

    cout<<"Enter your email: ";
    getline(cin, uemail);
    cout<<"Enter Password: ";
    getline(cin, upassword);

    ifstream in("users.txt");

    if (!in) {
        cout << RED << "File opening failed!\n" << RESET;
        return "";
    }

    getline(in, line);

    while(getline(in, id, '|') && getline(in, name, '|') &&  getline(in, email, '|') && getline(in, password, '|') && getline(in, role, '|')){
            if(trim(email) == trim(uemail) && trim(password) == upassword){
                flag=true;
                role = trim(role);
                userID = stoi(trim(id));
                User u(userID, trim(name), trim(email), trim(password), role);
                cout << GREEN << "Welcome " << role << RESET << endl;
                u.display();
                in.close();
                return role;
            }
    }

    in.close();

    if(flag == false){
        cout << RED << "Invalid email or password.\n" << RESET;
    }
    return "";
}

void listAllUsers(){
    string id, name, email, password, role, line;
    ifstream in("users.txt");

    if (!in) {
        cout << RED << "File opening failed!\n" << RESET;
        return;
    }

    printHeader("ALL USER DETAILS");
    cout << BOLD;
    writeUserHeader(cout);
    cout << RESET;
    printLine('-');

    getline(in, line);

    while(getline(in, id, '|') &&
        getline(in, name, '|') &&
        getline(in, email, '|') &&
        getline(in, password, '|') &&
        getline(in, role, '|') ){

        id = trim(id);
        name = trim(name);
        email = trim(email);
        password = trim(password);
        role = trim(role);

        cout << left
             << setw(5) << id
             << setw(3) << "|"
             << setw(15) << name
             << setw(3) << "|"
             << setw(20) << email
             << setw(3) << "|"
             << setw(15) << password
             << setw(3) << "|"
             << setw(15) << role << '\n';
    }

    printLine('-');
    in.close();

}

void addUserToFile(int id, string name, string email, string password, string role="user"){

    bool needHeader = isFileEmpty("users.txt");

    ofstream out("users.txt", ios::app);

    if (!out) {
        throw runtime_error("users.txt could not be opened");
    }

    if (needHeader) writeUserHeader(out);

    out<<left<<setw(5)<<id;
    out<<left<<setw(3)<<"|";
    out<<left<<setw(15)<<name;
    out<<left<<setw(3)<<"|";
    out<<left<<setw(20)<<email;
    out<<left<<setw(3)<<"|";
    out<<left<<setw(15)<<password;
    out<<left<<setw(3)<<"|";
    out<<left<<setw(15)<<role;
    out<<left<<setw(3)<<"|"<<endl;

    out.close();
    out.clear();
}

// ---------- Menus ----------

void adminMenu(){
    int choice;
    do {
        printHeader("ADMIN MENU");
        cout << "  1. Add Book\n";
        cout << "  2. Update Book\n";
        cout << "  3. Delete Book\n";
        cout << "  4. View All Books\n";
        cout << "  5. View All Users\n";
        cout << "  6. View Issued Books\n";
        cout << "  0. Logout\n";
        printLine();
        choice = readInt("Enter your choice: ");

        try {
            switch (choice) {
                case 1: addBook(); break;
                case 2: updateBook(); break;
                case 3: deleteBook(); break;
                case 4: listBooks(); break;
                case 5: listAllUsers(); break;
                case 6: listIssuedBooks(0); break;
                case 0: cout << YELLOW << "Logged out.\n" << RESET; break;
                default: cout << RED << "Invalid choice!\n" << RESET;
            }
        } catch (exception &e) {
            cout << RED << "Error: " << e.what() << RESET << endl;
        }

        if (choice != 0) waitEnter();
    } while (choice != 0);
}

void userMenu(int userID){
    int choice;
    do {
        printHeader("USER MENU");
        cout << "  1. View All Books\n";
        cout << "  2. Issue Book\n";
        cout << "  3. Return Book\n";
        cout << "  4. My Issued Books\n";
        cout << "  0. Logout\n";
        printLine();
        choice = readInt("Enter your choice: ");

        try {
            switch (choice) {
                case 1: listBooks(); break;
                case 2: issueBook(userID); break;
                case 3: returnBook(userID); break;
                case 4: listIssuedBooks(userID); break;
                case 0: cout << YELLOW << "Logged out.\n" << RESET; break;
                default: cout << RED << "Invalid choice!\n" << RESET;
            }
        } catch (exception &e) {
            cout << RED << "Error: " << e.what() << RESET << endl;
        }

        if (choice != 0) waitEnter();
    } while (choice != 0);
}

int main() {

    srand(time(0));

    try {
        // first run: create a default admin account
        if (isFileEmpty("users.txt")) {
            addUserToFile(100, "Admin", "admin@library.com", "admin123", "admin");
        }
    } catch (exception &e) {
        cout << RED << "Error: " << e.what() << RESET << endl;
        return 1;
    }

    int choice;
    do {
        printHeader("LIBRARY MANAGEMENT SYSTEM");
        cout << "  1. Login\n";
        cout << "  2. Register\n";
        cout << "  0. Exit\n";
        printLine();
        choice = readInt("Enter your choice: ");

        try {
            if (choice == 1) {
                int userID = 0;
                string role = loginUser(userID);
                if (role != "") waitEnter();
                if (role == "admin") adminMenu();
                else if (role == "user") userMenu(userID);
            }
            else if (choice == 2) {
                registerUser();
            }
            else if (choice == 0) {
                cout << CYAN << "Thank you! Goodbye.\n" << RESET;
            }
            else {
                cout << RED << "Invalid choice!\n" << RESET;
            }
        } catch (exception &e) {
            cout << RED << "Error: " << e.what() << RESET << endl;
        }

        if (choice != 0) waitEnter();
    } while (choice != 0);

    return 0;
}
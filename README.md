# Library Management System (C++)

A simple console-based Library Management System written in C++. It uses plain text files for storage, so no database is needed. Users can register and log in, admins manage the book catalogue, and users can issue and return books.

---

## Features

- Register and login with role based access (**admin** / **user**)
- Auto-generated random 3-digit IDs for users and books
- **Admin:** add, update and delete books, view all books, view all users, view all issued books
- **User:** view all books, issue books, return books, view own issued books
- Colourful, centered CLI output with `====` and `----` separators
- Input validation and exception handling (no crash on wrong input or file problems)

---

## How to compile and run

```bash
g++ LMS.cpp -o library
./library.exe
```

Colours use ANSI codes, so on Windows use Windows Terminal or a recent console.

**Default admin account** (created automatically on the first run):

| Email             | Password |
|-------------------|----------|
| admin@library.com | admin123 |

---

## Data files

All files use the same style: a header row, then one record per line, with columns separated by `|`. The program creates them automatically.

| File | Purpose | Columns |
|------|---------|---------|
| `users.txt` | All registered users | ID, Name, Email, Password, Role |
| `books.txt` | Book catalogue and stock | ID, Title, Author, Publication, Quantity |
| `issued_books.txt` | Books currently issued | Book ID, User ID, Issue Date, Issued, Stock Left |
| `temp.txt` | Temporary file used while rewriting a file (removed automatically) | - |

---

## Program flow

```
START
  |
  +-- first run? create users.txt with default admin
  |
  +-- MAIN MENU
        1. Login ----> check email + password in users.txt
        |                 |
        |                 +-- role = admin --> ADMIN MENU
        |                 +-- role = user  --> USER MENU (user ID taken from login)
        |                 +-- wrong details --> "Invalid email or password"
        2. Register --> auto ID, name, email, password -> saved as role "user"
        0. Exit
```

### Admin menu

| Option |                   Action                               | 
|--------|--------------------------------------------------------|
|    1   | Add Book (ID is auto-generated)                        |
|    2   | Update Book (title, author, publication, quantity)     |
|    3   | Delete Book (blocked if copies are currently issued)   |
|    4   | View All Books                                         |
|    5   | View All Users                                         |
|    6   | View Issued Books (all users)                          |
|    0   | Logout                                                 |

### User menu

| Option | Action             |
|--------|--------------------|
|    1   | View All Books     |
|    2   | Issue Book         |
|    3   | Return Book        |
|    4   | My Issued Books    |
|    0   | Logout             |

---

## Issue and Return logic

**Issue**
1. The user picks a book ID and the number of copies.
2. Stock in `books.txt` is reduced. If the book does not exist or there are not enough copies, it is refused.
3. In `issued_books.txt`:
   - if this user already has this book, the Issued count increases and the date is refreshed
   - otherwise a new entry is created

**Return**
1. The user's issued books are shown and the user picks a book ID.
2. If the user has not issued that book, it **cannot be returned**.
3. The user cannot return more copies than they hold.
4. Stock in `books.txt` is increased, and the entry in `issued_books.txt` is reduced. When it reaches 0 the entry is removed.

`books.txt` always holds the real stock. The **Stock Left** column in `issued_books.txt` is refreshed on every issue and return for all rows of that book.

---

## Code structure (functions)

### ID generation
| Function | What it does |
|----------|--------------|
| `idExists(file, id, hasHeader)` | Checks if an ID is already used in a file |
| `generateID(file)` | Returns a random unused ID between 100 and 999 |

### Classes
| Class | Purpose |
|-------|---------|
| `User` | Holds id, name, email, password, role. `display()` shows the details |
| `Book` | Holds book data. Its constructor saves the book to `books.txt` (writing the header if the file is new) |

### Book functions
| Function | What it does |
|----------|--------------|
| `addBook()` | Asks for the details and saves a new book with an auto ID |
| `updateBook()` | Changes a book's details; Enter keeps a field unchanged |
| `deleteBook()` | Removes a book (not allowed while it is issued) |
| `listBooks()` | Shows all books in a table |

### Issue / Return functions
| Function | What it does |
|----------|--------------|
| `issueBook(userID)` | Issues copies of a book to the logged-in user |
| `returnBook(userID)` | Returns copies of a book issued by the logged-in user |
| `listIssuedBooks(userID)` | Shows issued books (`0` = everyone's, otherwise only that user's) |

### User functions
| Function | What it does |
|----------|--------------|
| `registerUser()` | Creates a new user with an auto ID and role `user` |
| `loginUser(userID)` | Checks credentials, returns the role (empty if failed) and fills in the user ID |
| `listAllUsers()` | Shows all users in a table |
| `addUserToFile(...)` | Appends a user to `users.txt` (writes the header if the file is new) |

### Menus and main
| Function | What it does |
|----------|--------------|
| `adminMenu()` | Menu loop for the admin |
| `userMenu(userID)` | Menu loop for a normal user |
| `main()` | Creates the default admin on first run and runs the main menu |

---

## Error handling

- Number inputs are checked, so letters or empty input just ask again
- Failures to open files are reported instead of crashing
- `stoi` conversions are protected with `try / catch`
- Every menu action is wrapped in `try / catch` and shows `Error: ...` instead of closing
- Closing the input (Ctrl+D) exits the program cleanly

---

## Limitations

- Passwords are stored in plain text (hashing could be added later)
- The `|` character must not be used in names, titles, etc., because it separates the columns
- Only one admin exists by default; others can be created by editing the role in `users.txt`
- If an admin changes a book's quantity with Update, the Stock Left value in `issued_books.txt` refreshes at the next issue/return of that book

/*
  This file is part of KDBindings.

  SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Leon Matthes <leon.matthes@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include <ios>
#include <kdbindings/signal.h>

#include <iostream>
#include <string>

using namespace KDBindings;

void displayLabelled(const std::string &label, int value)
{
    std::cout << label << ": " << value << std::endl;
}

int main()
{
    Signal<int> signal;

    {
        // A ScopedConnection will disconnect the connection once it goes out of scope.
        // It is especially useful if you're connecting to a member function.
        // Storing a ScopedConnection in the object that contains the slot ensures the connection
        // is disconnected when the object is destructed.
        // This ensures that there are no dangling connections.
        ScopedConnection guard = signal.connect(displayLabelled, "Guard is connected");

        signal.emit(1);
    } // The connection is disconnected here

    signal.emit(2);

    ConnectionHandle handle = signal.connect(displayLabelled, "Connection is not blocked");

    signal.emit(3);
    {
        // A ConnectionBlocker will block a connection for the duration of its scope.
        // This is a good way to avoid endless-recursion, or to suppress updates for a short time.
        ConnectionBlocker blocker(handle); // The connection is blocked here

        signal.emit(4);
    } // The connection is un-blocked here

    signal.emit(5);

    return 0;
}

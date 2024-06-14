/*
  This file is part of KDBindings.

  SPDX-FileCopyrightText: 2021 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sean Harmer <sean.harmer@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include <kdbindings/signal.h>

#include <iostream>
#include <string>

using namespace KDBindings;

class Person
{
public:
    Person(std::string const &name)
        : m_name(name)
    {
    }

    Signal<std::string const &> speak;

    void listen(std::string const &message)
    {
        std::cout << m_name << " received: " << message << std::endl;
    }

private:
    std::string m_name;
};

int main()
{
    Person alice("Alice");
    Person bob("Bob");

    auto connection1 = alice.speak.connect(&Person::listen, &bob);
    auto connection2 = bob.speak.connect(&Person::listen, &alice);

    alice.speak.emit("Have a nice day!");
    bob.speak.emit("Thank you!");

    connection1.disconnect();
    connection2.disconnect();

    return 0;
}

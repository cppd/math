/*
Copyright (C) 2017-2020 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "connection.h"

#include <QObject>

namespace ns::gui
{
void Connection::destroy() noexcept
{
        if (m_connection)
        {
                QObject::disconnect(*m_connection);
        }
}

void Connection::move(Connection* from) noexcept
{
        m_connection = from->m_connection;
        from->m_connection.reset();
}

Connection::Connection(QMetaObject::Connection&& connection) : m_connection(std::move(connection))
{
}

Connection::~Connection()
{
        destroy();
}

Connection::Connection(Connection&& obj) noexcept
{
        move(&obj);
}

Connection& Connection::operator=(Connection&& obj) noexcept
{
        if (this != &obj)
        {
                destroy();
                move(&obj);
        }
        return *this;
}
}

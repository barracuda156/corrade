#ifndef Corrade_Utility_Test_DirectoryTest_h
#define Corrade_Utility_Test_DirectoryTest_h
/*
    Copyright © 2007, 2008, 2009, 2010, 2011 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Corrade.

    Corrade is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Corrade is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include <QtCore/QObject>

namespace Corrade { namespace Utility { namespace Test {

class DirectoryTest: public QObject {
    Q_OBJECT

    private slots:
        void path_data();
        void path();

        void filename_data();
        void filename();

        void join_data();
        void join();

        void mkpath_data();
        void mkpath();

        void remove_data();
        void remove();

        void fileExists();

        void home();
        void configurationDir();

        void list_data();
        void list();
};

}}}

#endif

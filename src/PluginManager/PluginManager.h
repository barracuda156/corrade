#ifndef Corrade_Plugins_PluginManager_h
#define Corrade_Plugins_PluginManager_h
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

/** @file
 * @brief Class Corrade::PluginManager::PluginManager
 */

#ifndef CORRADE_SKIP_PLUGINMANAGER_NAMESPACE
#include <string>
#include <vector>

#include "AbstractPluginManager.h"

namespace Corrade { namespace PluginManager {
#endif

/**
 * @brief %Plugin manager
 *
 * Manages loading, instancing and unloading plugins.
 * See also @ref PluginManagement.
 */
template<class T> class PluginManager: public AbstractPluginManager {
    public:
        /** @copydoc AbstractPluginManager::AbstractPluginManager() */
        PluginManager(const std::string& pluginDirectory): AbstractPluginManager(pluginDirectory) {
            /* Find static plugins which have the same interface and have not
               assigned manager to them */
            for(std::map<std::string, PluginObject*>::iterator it = plugins()->begin(); it != plugins()->end(); ++it) {
                if(it->second->manager != 0 || it->second->interface != pluginInterface())
                    continue;

                /* Assign the plugin to this manager */
                it->second->manager = this;
            }
        }

        std::string pluginInterface() const { return T::pluginInterface(); }

        /**
         * @brief %Plugin class instance
         * @param _plugin           %Plugin
         * @return Pointer to new instance of plugin class, zero on error
         *
         * Creates new instance of plugin class, if possible. If the plugin is
         * not successfully loaded, returns zero pointer.
         */
        T* instance(const std::string& _plugin) {
            /* Plugin with given name doesn't exist */
            if(plugins()->find(_plugin) == plugins()->end()) return 0;

            PluginObject& plugin = *plugins()->at(_plugin);

            /* Plugin is not successfully loaded */
            if(!(plugin.loadState & (LoadOk|IsStatic))) return 0;

            return static_cast<T*>(plugin.instancer(this, _plugin));
        }
};

#ifndef CORRADE_SKIP_PLUGINMANAGER_NAMESPACE
}}
#endif

#endif

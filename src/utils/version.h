/*
 * Waifu Gallery - A Qt-based image gallery application.
 * Copyright (C) 2025 R4nd5tr <r4nd5tr@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define WG_VERSION_MAJOR 0
#define WG_VERSION_MINOR 0
#define WG_VERSION_PATCH 0
#define WG_VERSION_STATUS "alpha"
#define WG__STRINGIFY(x) #x
#define WG_STRINGFY(x) WG__STRINGIFY(x)

#define WG_VERSION WG_STRINGFY(WG_VERSION_MAJOR.WG_VERSION_MINOR.WG_VERSION_PATCH) WG_VERSION_STATUS


ifeq ($(NDK_MAJOR),)
APP_CFLAGS   := -std=c17
APP_CPPFLAGS := -std=c++17 -fexceptions -frtti
else
ifeq ($(NDK_MAJOR),21)
# clang 9
APP_CFLAGS   := -std=c17
APP_CPPFLAGS := -std=c++17 -fexceptions -frtti
else ifeq ($(NDK_MAJOR),22)
# clang 11
APP_CFLAGS   := -std=c17
APP_CPPFLAGS := -std=c++20 -fexceptions -frtti
else ifeq ($(NDK_MAJOR),23)
# clang 12
APP_CFLAGS   := -std=c17
APP_CPPFLAGS := -std=c++20 -fexceptions -frtti
else ifeq ($(NDK_MAJOR),24)
# clang 14
APP_CFLAGS   := -std=c17
APP_CPPFLAGS := -std=c++20 -fexceptions -frtti
else ifeq ($(NDK_MAJOR),25)
# clang 14
APP_CFLAGS   := -std=c17
APP_CPPFLAGS := -std=c++20 -fexceptions -frtti
else ifeq ($(NDK_MAJOR),26)
# clang 17
APP_CFLAGS   := -std=c17
APP_CPPFLAGS := -std=c++20 -fexceptions -frtti
else ifeq ($(NDK_MAJOR),27)
# clang 18
APP_CFLAGS   := -std=c17
APP_CPPFLAGS := -std=c++20 -fexceptions -frtti
else
APP_CFLAGS   := -std=c17
APP_CPPFLAGS := -std=c++20 -fexceptions -frtti
endif
endif

APP_LDFLAGS  := 
APP_STL      := c++_shared

APP_SUPPORT_FLEXIBLE_PAGE_SIZES := true

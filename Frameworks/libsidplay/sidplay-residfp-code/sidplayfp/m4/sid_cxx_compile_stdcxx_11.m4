# Test for std c++11 compiler support
#
# trimmed down verision of AX_CXX_COMPILE_STDCXX_11
# from the GNU Autoconf Archive
#
#   Copyright (c) 2008 Benjamin Kosnik <bkoz@redhat.com>
#   Copyright (c) 2012 Zack Weinberg <zackw@panix.com>
#   Copyright (c) 2013 Roy Stogner <roystgnr@ices.utexas.edu>
#   Copyright (c) 2014 Alexey Sokolov <sokolov@google.com>
#   Copyright (c) 2014, 2015 Google Inc.
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 7

m4_define([_AX_CXX_COMPILE_STDCXX_11_testbody], [[
  template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

    struct Base {
    virtual void f() {}
    };
    struct Child : public Base {
    virtual void f() override {}
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c;
    check_type&& cr = static_cast<check_type&&>(c);

    auto d = a;
    auto l = [](){};

    // http://stackoverflow.com/questions/13728184/template-aliases-and-sfinae
    // Clang 3.1 fails with headers of libstd++ 4.8.3 when using std::function because of this
    namespace test_template_alias_sfinae {
        struct foo {};

        template<typename T>
        using member = typename T::member_type;

        template<typename T>
        void func(...) {}

        template<typename T>
        void func(member<T>*) {}

        void test() {
            func<foo>(0);
        }
    }
]])

AC_DEFUN([SID_CXX_COMPILE_STDCXX_11], [dnl
  AC_LANG_PUSH([C++])dnl
  ac_success=no
  AC_CACHE_CHECK(whether $CXX supports C++11 features by default,
  ax_cv_cxx_compile_cxx11,
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_testbody])],
    [ax_cv_cxx_compile_cxx11=yes],
    [ax_cv_cxx_compile_cxx11=no])])
  if test x$ax_cv_cxx_compile_cxx11 = xyes; then
    ac_success=yes
  fi

  if test x$ac_success = xno; then
      cachevar=AS_TR_SH([ax_cv_cxx_compile_cxx11_-std=c++11])
      AC_CACHE_CHECK(whether $CXX supports C++11 features with -std=c++11,
                     $cachevar,
        [ac_save_CXXFLAGS="$CXXFLAGS"
         CXXFLAGS="$CXXFLAGS -std=c++11"
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_testbody])],
          [eval $cachevar=yes],
          [eval $cachevar=no])
         CXXFLAGS="$ac_save_CXXFLAGS"])
      if eval test x\$$cachevar = xyes; then
        CXXFLAGS="$CXXFLAGS -std=c++11"
        ac_success=yes
        break
      fi
  fi

  AC_LANG_POP([C++])

  if test x$ac_success = xno; then
    HAVE_CXX11=0
    AC_MSG_NOTICE([No compiler with C++11 support was found])
  else
    HAVE_CXX11=1
    AC_DEFINE(HAVE_CXX11,1,
              [define if the compiler supports basic C++11 syntax])
  fi

  AC_SUBST(HAVE_CXX11)
])

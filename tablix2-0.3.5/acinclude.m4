# Macros for PVM3
# Tomaz Solc

dnl AM_PATH_PVM3([action-if-found], [action-if-not-found])

AC_DEFUN([AM_PATH_PVM3],[
        no_pvm="no"

        AC_MSG_CHECKING(for PVM_ROOT)
        if test "x$PVM_ROOT" = "x" ; then
                AC_MSG_RESULT(no)

                AC_CHECK_LIB(pvm3, pvm_spawn, dummy=1, [no_pvm=yes])
                AC_CHECK_HEADER(pvm3.h, [], [no_pvm=yes])

                PVM_LIBS="-lpvm3"
        else
                AC_MSG_RESULT("$PVM_ROOT")

                AC_PATH_PROG(GETARCH, "pvmgetarch", no, "$PVM_ROOT/lib:$PATH")

                AC_MSG_CHECKING("for PVM_ARCH")

                if test "x$PVM_ARCH" = "x" ; then
                        if test "$GETARCH" = "no" ; then
                                AC_MSG_RESULT("no")
                                no_pvm="yes"
                        else
                                PVM_ARCH=`$GETARCH`
                                AC_MSG_RESULT("$PVM_ARCH")
                        fi
                else
                        AC_MSG_RESULT("$PVM_ARCH");
                fi

                if test "x$no_pvm" = "xno" ; then
                        PVM_CFLAGS="-I$PVM_ROOT/include"
                        PVM_LIBS="-L$PVM_ROOT/lib/$PVM_ARCH -lpvm3"

                        ac_save_CFLAGS="$CFLAGS"
                        CFLAGS="$CFLAGS -L$PVM_ROOT/lib/$PVM_ARCH"
                        AC_CHECK_LIB(pvm3, pvm_spawn, dummy=1, no_pvm=yes)
                        CFLAGS="$ac_save_CFLAGS"

			ac_save_CPPFLAGS="$CPPFLAGS"
                        CPPFLAGS="$CPPFLAGS $PVM_CFLAGS"
                        AC_CHECK_HEADER(pvm3.h, [], no_pvm=yes)
                        CPPFLAGS="$ac_save_CPPFLAGS"
                fi
        fi

        if test "x$no_pvm" = "xyes" ; then
                AC_MSG_WARN([Parallel algorithm disabled])
                PVM_LIBS=""
                PVM_CFLAGS=""
	fi

	AS_IF([test "x$no_pvm" = "xyes"], [$2], [$1])

        AC_SUBST(PVM_LIBS)
        AC_SUBST(PVM_CFLAGS)

])

# Code from AC_LTDL_SHLIBEXT (libltdl)
#
# defines LTDL_SHLIB_EXT which contains shared library extension

AC_DEFUN([AC_SHLIB_EXT],[
	AC_REQUIRE([AC_LIBTOOL_SYS_DYNAMIC_LINKER])
	
	AC_CACHE_CHECK([which extension is used for loadable modules],
  		[libltdl_cv_shlibext],
		[
			module=yes
			eval libltdl_cv_shlibext=$shrext_cmds
  		])
	
	if test -n "$libltdl_cv_shlibext"; then
  		AC_DEFINE_UNQUOTED([LTDL_SHLIB_EXT], ["$libltdl_cv_shlibext"],
	    [Define to the extension used for shared libraries, say, ".so".])
	fi
])


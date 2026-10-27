v3d_add_api_library(database "Binder.h" "Binder.cpp" "Columns.h" "Columns.cpp" "Database.h" "Database.cpp"
	"Statement.h" "Statement.cpp" "Transaction.h" "Transaction.cpp")

# no header includes sqlite3.h, so the package stays private to the library
target_link_libraries(v3dlib_database PRIVATE unofficial::sqlite3::sqlite3)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()

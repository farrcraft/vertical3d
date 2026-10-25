v3d_add_test(database
	"TestMain.cpp"
	"DatabaseTest.cpp"
	"StatementTest.cpp"
	"TransactionTest.cpp")

target_link_libraries(v3dtest_database PRIVATE v3dlib_database Boost::filesystem)

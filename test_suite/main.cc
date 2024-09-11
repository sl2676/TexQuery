#include "main.h"
#include "./src/TestSuite.h"
wxIMPLEMENT_APP(TestSuiteScreen);

bool TestSuiteScreen::OnInit() {
	TestSuite* frame = new TestSuite(
		"GULASEARCH_TEST_SUITE",
		wxPoint(100, 100),
		wxSize(900, 690)
	);
	frame->Show(true);
	return true;
};

#include "./PromptWindow.h"

PromptWindow::PromptWindow(
	const string& title,
	const wxPoint& pos,
	const wxSize& size
) : wxFrame (
	NULL,
	wxID_ANY,
	title,
	pos,
	size,
	wxDEFAULT_FRAME_STYLE
) {
	
}

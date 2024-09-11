#include "./TextWindow.h"

TextWindow::TextWindow(
	const wxString& title,
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
	
};

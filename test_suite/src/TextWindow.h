#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif
#ifndef TEXT_WINDOW_H
#define TEXT_WINDOW_H

class TextWindow : public wxFrame {
public:
	TextWindow(
		const wxString& title,
		const wxPoint& pos,
		const wxSize& size
	);

};

#endif


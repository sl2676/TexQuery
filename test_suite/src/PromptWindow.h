#ifndef 
	#include <wx/wx.h>
#endif

#ifndef PROMPT_WINDOW_H
#define PROMPT_WINDOW_H

class PromptWindow : public wxFrame {
public:
	PromptWindow(
		const wxString& title,
		const wxPoint& pos,
		const wxSize& size
	);
};

#endif


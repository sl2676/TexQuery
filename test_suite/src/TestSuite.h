//
// Created by Sean Ly 2/10/24
//

#ifndef WX_PRECOMP
	#include <wx/wx.h>
	#include <wx/textctrl.h>
	#include <wx/font.h>
	#include <wx/textfile.h>
	#include <wx/utils.h>
	#include <wx/wxchar.h>
#endif
#include <iostream>
#include "./id_cat.h"
#include "./TextWindow.h"
#include "./PromptWindow.h"
#ifndef TEST_SUITE_FRAME
#define TEST_SUITE_FRAME

class TestSuite : public wxFrame {
public:
	TestSuite(
		const wxString& title,
		const wxPoint& pos,
		const wxSize& size
	);

private:
	wxDECLARE_EVENT_TABLE();
	wxString _str_file_path;
	wxString _str_file_contents;
	
	wxWindow* _window_controls;
	wxTextCtrl* _text_ctrl_latex_input;
	wxButton* _button_latex_file_input;
	wxButton* _button_run;

	void OnAbout(wxCommandEvent& event);
	void FileInput(wxCommandEvent& event);
	void Run(wxCommandEvent& event);
};

#endif

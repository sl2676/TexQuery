#include "./TestSuite.h"

TestSuite::TestSuite(
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size	
) : wxFrame(
	NULL,
	wxID_ANY,
	title,
	pos,
	size,
	wxDEFAULT_FRAME_STYLE
	& ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX)
) {
	_window_controls = new wxWindow(
		this,
		wxID_ANY,
		wxPoint(100, 100),
		wxSize(600, 600)
	);
	_window_controls->SetBackgroundColour(wxColor(67, 70, 75));
	

	_text_ctrl_latex_input = new wxTextCtrl(
		_window_controls,
		wxID_ANY,
		_T(""),
		wxPoint(260, 70),
		wxSize(450, 450),
		wxTE_RICH | wxTE_RICH2 | wxHSCROLL |
		wxTE_PROCESS_TAB | wxSB_HORIZONTAL |
		wxTE_PROCESS_ENTER | wxTE_MULTILINE
	);
	_button_latex_file_input = new wxButton(
		_window_controls,
		ID_FILE_INPUT,
		_str_file_path,
		wxPoint(280, 30),
		wxSize(400, 25)
	);	
	
	_button_run = new wxButton(
		_window_controls,
		ID_RUN,
		_T("RUN"),
		wxPoint(600, 500),
		wxSize(100, 100)
	);
	
}
wxBEGIN_EVENT_TABLE(TestSuite, wxFrame)
	EVT_BUTTON(ID_FILE_INPUT, TestSuite::FileInput)
	EVT_BUTTON(ID_RUN, TestSuite::Run)
wxEND_EVENT_TABLE()

void TestSuite::FileInput(wxCommandEvent& event) {
	wxString file;
	wxString str;
	wxTextFile tfile;
	wxFileDialog fdlog(this);
	
	if (fdlog.ShowModal() != wxID_OK) return;
	
	file.Clear();
	file = fdlog.GetPath();
	tfile.Open(file);
	str = tfile.GetFirstLine();
	
	while (!tfile.Eof()) {
		str = tfile.GetNextLine();
		_text_ctrl_latex_input->AppendText(str);
		_text_ctrl_latex_input->AppendText('\n');
	}
	//std::cout << fdlog.GetPath() << std::endl;
	_button_latex_file_input->SetLabel(fdlog.GetPath());
	_str_file_path = fdlog.GetPath();
};

void TestSuite::Run(wxCommandEvent& event) {
	wxString test_run = "python3 main.py " +
	_str_file_path;
	wxExecute(test_run, wxEXEC_ASYNC, NULL, NULL);
	TextWindow* text_window = new TextWindow(
		"TEXT_WINDOW",
		wxPoint(50, 50),
		wxSize(500, 700)
	);
	text_window->SetBackgroundColour(wxColor(100, 100, 100));
	text_window->Show(true);
	PromptWindow& prompt_window = new PromptWindow(
		"PROMPT_WINDOW",
		wxPoint(200, 200),
		wxSize(300, 500)
	);
	prompt_window->SetBackgroundColour(wxColor(200, 200, 200));
	prompt_window->Show(true);

};

void TestSuite::OnAbout(wxCommandEvent& event) {
	wxMessageBox(
		"TestSuite GULASEARCH",
		"GULASEARCH",
		wxOK | wxICON_INFORMATION
	);
};

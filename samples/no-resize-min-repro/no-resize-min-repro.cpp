// std
#include <iostream>
#include <tuple>

// wxWidgets
#include <wx/textwrapper.h>
#include <wx/wx.h>

// return the wraped string and its extents
static std::tuple<wxString, wxSize> get_wrapped_string_size(
    wxWindow& win,
    const wxString& text,
    std::vector<wxString>& line_buffer,
    int max_width
) {
    struct wxStringWrapper: public wxTextWrapper {
    public:
        wxStringWrapper(
            wxWindow* win,
            const wxString& text,
            std::vector<wxString>& line_buffer,
            int max_width
        ) :
            line_buffer(line_buffer) {
            this->line_buffer.clear();
            this->Wrap(win, text, max_width);
        }

        wxString get_text() const {
            wxString result;
            result.Alloc(this->size);

            const wxString NEWLINE("\n");
            if (auto it = this->line_buffer.begin(); it != this->line_buffer.end()) {
                result.Append(*it);

                for (++it; it != this->line_buffer.end(); ++it) {
                    result.Append(NEWLINE);
                    result.Append(*it);
                }
            }
            return result;
        }

    protected:
        void OnOutputLine(const wxString& line) override {
            this->line_buffer.push_back(line);
            this->size += line.Len();
        }

        void OnNewLine() override {
            this->size += 1;
        }

    private:
        size_t size = 0;
        std::vector<wxString>& line_buffer;
    };

    wxStringWrapper wrapper(&win, text, line_buffer, max_width);
    auto wrapped_text = wrapper.get_text().Trim();

    const wxClientDC dc(&win);
    auto size = dc.GetMultiLineTextExtent(wrapped_text);

    return std::make_tuple(wrapped_text, size);
}

static int COUNTER = 0;

// wrapper around a wxPanel containing a single child wxStaticText
// whose size is set by a parent sizer
class WrappedStaticText: public wxPanel {
public:
    WrappedStaticText(
        wxWindow* parent,
        wxWindowID id,
        const wxString& label,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize
    ) :
        wxPanel(parent, id, pos, size),
        label(label) {
        auto static_text = new wxStaticText(this, wxID_ANY, wxEmptyString);

        // debug background color
        switch (COUNTER++) {
            case 0:
                this->SetBackgroundColour(wxColour(0x88, 0xFF, 0xFF));
                break;
            case 1:
                this->SetBackgroundColour(wxColour(0xFF, 0x88, 0xFF));
                break;
            case 2:
                this->SetBackgroundColour(wxColour(0xFF, 0xFF, 0x88));
                break;
        }

        this->Bind(wxEVT_SIZE, [=, this](wxSizeEvent& evt) {
            const auto size = evt.GetSize();
            const auto width = size.GetWidth();

            wxString text;
            wxSize extent;
            std::tie(text, extent) =
                get_wrapped_string_size(*this, this->label, this->line_buffer, width);

            this->SetMinSize(wxSize(-1, extent.GetHeight()));
            static_text->SetLabel(text);

            this->Layout();
            evt.Skip();
        });
    }

    wxSize DoGetBestSize() const override {
        return wxSize(1, 1);
    }

private:
    const wxString label;
    std::vector<wxString> line_buffer;
};

class MainFrame: public wxFrame {
public:
    MainFrame() : wxFrame(nullptr, wxID_ANY, "no-resize-min-repro") {
        const auto lorem_ipsum = wxString(
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
        );

        auto panel = new wxPanel(this, wxID_ANY);

        auto top_text = new WrappedStaticText(panel, wxID_ANY, lorem_ipsum);
        auto left_text = new WrappedStaticText(panel, wxID_ANY, lorem_ipsum);
        auto right_text = new WrappedStaticText(panel, wxID_ANY, lorem_ipsum);

        auto v_sizer = new wxBoxSizer(wxVERTICAL);

        auto h_sizer = new wxBoxSizer(wxHORIZONTAL);
        v_sizer->Add(top_text, 0, wxEXPAND | wxALL, 8);

        h_sizer->Add(left_text, 1, wxEXPAND | wxRIGHT, 4);
        h_sizer->Add(right_text, 2, wxEXPAND | wxLEFT, 4);

        v_sizer->Add(h_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        panel->SetSizer(v_sizer);

        this->SetSize(wxSize(800, 600));
    }
};

class App: public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit()) {
            return false;
        }

        auto main_frame = new MainFrame();
        main_frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(App);
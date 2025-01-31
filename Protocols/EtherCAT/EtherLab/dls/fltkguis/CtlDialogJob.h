/******************************************************************************
 *
 *  $Id$
 *
 *  This file is part of the Data Logging Service (DLS).
 *
 *  DLS is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef CtlDialogJobH
#define CtlDialogJobH

/*****************************************************************************/

#include <vector>
using namespace std;

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>

/*****************************************************************************/

#include "fl_grid.h"
#include "CtlGlobals.h"
#include "CtlJobPreset.h"

/*****************************************************************************/

/**
   Dialog zum bearbeiten einer Auftragsvorgabe
*/

class CtlDialogJob
{
public:
    CtlDialogJob(const string &);
    ~CtlDialogJob();

    void show(CtlJobPreset *);
    bool updated() const;
    bool imported() const;

private:
    Fl_Double_Window *_wnd; /**< Dialogfenster */
    Fl_Button *_button_close; /**< "Schliessen" - Button */
    Fl_Output *_output_desc; /**< Ausgabefeld f�r die Auftragsbeschreibung */
    Fl_Output *_output_source; /**< Ausgabefeld f�r die Datenquelle */
    Fl_Output *_output_trigger; /**< Ausgabefeld f�r den Namen des
                                   Trigger-Parameters */
    Fl_Button *_button_change; /**< Button f�r den Dialog zur �nderung
                                  der Beschreibung, usw. */
    Fl_Grid *_grid_channels; /**< Das Grid zur Darstellung der Kan�le */
    Fl_Button *_button_add; /**< Button zum Hinzuf�gen von Kan�len */
    Fl_Button *_button_rem; /**< Button zum entfernen von gew�hlten Kan�len */
    Fl_Button *_button_edit; /**< Button zum Editieren von gew�hlten Kan�len */
    string _dls_dir; /**< DLS-Datenverzeichnis */
    CtlJobPreset *_job; /**< Zeiger auf den zu bearbeitenden Auftrag */
    bool _changed; /**< Flag, das anzeigt, ob etwas am
                      Auftrag ge�ndert wurde */
    bool _updated; /**< Flag, das angibt, ob der Auftrag
                      ge�ndert gespeichert wurde */

    static void _callback(Fl_Widget *, void *);
    void _grid_channels_callback();
    void _button_close_clicked();
    void _button_change_clicked();
    void _button_add_clicked();
    void _button_rem_clicked();
    void _button_edit_clicked();

    bool _load_channels();

    bool _save_job();
    void _edit_channels();
    void _insert_channels(const list<LibDLS::RealChannel> *);
    void _remove_channels(const list<unsigned int> *);

    void _grid_selection_changed();
};

/*****************************************************************************/

/**
   Gibt zur�ck, ob der Auftrag ge�ndert und gespeichert wurde

   \return true, wenn ge�ndert und gespeichert
*/

inline bool CtlDialogJob::updated() const
{
    return _updated;
}

/*****************************************************************************/

#endif

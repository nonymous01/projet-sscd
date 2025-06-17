import json
import os
import signal
import pandas as pd
from dash import Dash, dcc, html, Input, Output, State, ctx, dash_table
import dash_bootstrap_components as dbc
from datetime import datetime

# Palette de couleurs
COLOR_BG = '#0C0F0A'
COLOR_TEXT = '#FFFFFF'
COLOR_OK = '#47EAD0'
COLOR_WARN = '#FCFE19'
COLOR_CRIT = '#FE277E'

def load_data():
    with open('moniteur/output.json') as f:
        data = json.load(f)
    df = pd.DataFrame(data)
    df['status'] = df.apply(lambda row: get_status(row['cpu_percent'], row['mem_percent']), axis=1)
    return df

def get_status(cpu, mem):
    if cpu > 80 or mem > 80:
        return 'CRITIQUE'
    elif cpu > 50 or mem > 50:
        return 'ALERTE'
    else:
        return 'NORMAL'

def load_fifo_data():
    # Charge les données JSON de l'ordonnanceur, retourne valeurs par défaut si erreur
    try:
        with open('ordonnanceur_output.json') as f:
            return json.load(f)
    except Exception:
        return {
            "tasks_in_queue": [],
            "current_task": None,
            "tasks_processed": 0,
            "average_wait_time": 0,
            "throughput": 0
        }

app = Dash(__name__, external_stylesheets=[dbc.themes.DARKLY], suppress_callback_exceptions=True)
app.title = "Moniteur Système - Dashboard"

pagination_slider = lambda slider_id: html.Div([
    html.Label("Page :", style={'color': COLOR_TEXT}),
    dcc.Slider(id=slider_id, min=1, max=1, step=1, value=1, marks={}, tooltip={"placement": "bottom"})
], style={'marginBottom': '20px'})

app.layout = dbc.Container([
    html.H1("Dashboard Moniteur Système", style={'color': COLOR_TEXT, 'textAlign': 'center'}),

    html.Div([
        html.Label("Trier par :", style={'color': COLOR_TEXT}),
        dcc.RadioItems(
            id='sort-mode',
            options=[
                {'label': 'CPU', 'value': 'cpu_percent'},
                {'label': 'Mémoire', 'value': 'mem_percent'}
            ],
            value='cpu_percent',
            inline=True,
            style={'color': COLOR_TEXT}
        )
    ], style={'marginBottom': '20px'}),

    pagination_slider('page-slider-top'),

    dcc.Store(id='selected-pid', data=None),
    dcc.Store(id='killed-pid-store', data=[]),
    dcc.Interval(id='interval-component', interval=1000, n_intervals=0),

    dash_table.DataTable(
        id='process-table',
        columns=[
            {'name': 'PID', 'id': 'pid'},
            {'name': 'Utilisateur', 'id': 'user'},
            {'name': 'Commande', 'id': 'cmd'},
            {'name': 'État', 'id': 'state'},
            {'name': '% CPU', 'id': 'cpu_percent'},
            {'name': '% Mémoire', 'id': 'mem_percent'},
            {'name': 'Statut', 'id': 'status'}
        ],
        page_size=15,
        style_data_conditional=[],
        style_cell={'textAlign': 'center', 'backgroundColor': COLOR_BG, 'color': COLOR_TEXT},
        style_header={'backgroundColor': '#1f2c56', 'color': 'white', 'fontWeight': 'bold'},
        row_selectable='single',
        filter_action='native',
        sort_action='native',
        style_table={'overflowX': 'auto'}
    ),

    pagination_slider('page-slider-bottom'),

    html.Div(id='selected-process', style={'color': COLOR_TEXT, 'marginTop': '20px'}),

    html.Button("🛑 Terminer le processus sélectionné", id='terminate-button', n_clicks=0,
                style={'backgroundColor': '#FE277E', 'color': '#FFFFFF', 'marginTop': '20px', 'padding': '10px 20px', 'borderRadius': '5px'}),

    html.Br(),
    html.Button("⬇️ Exporter vers CSV", id='export-button', n_clicks=0,
                style={'marginTop': '20px', 'backgroundColor': '#47EAD0', 'color': 'black'}),
    html.Div(id='export-status', style={'color': COLOR_TEXT, 'marginTop': '10px'}),

    # === NOUVELLE SECTION ORDONNANCEUR FIFO ===
    html.Hr(style={'marginTop': '40px', 'marginBottom': '20px', 'borderColor': COLOR_TEXT}),
    html.H3("Ordonnanceur FIFO", style={'color': COLOR_OK}),
    html.Div(id='fifo-info', style={'color': COLOR_TEXT, 'fontFamily': 'monospace', 'whiteSpace': 'pre-line'}),
    dcc.Interval(id='fifo-interval', interval=1000, n_intervals=0)
])

# === CALLBACKS MONITEUR ===

@app.callback(
    Output('process-table', 'data'),
    Output('process-table', 'style_data_conditional'),
    Output('page-slider-top', 'max'),
    Output('page-slider-top', 'marks'),
    Output('page-slider-bottom', 'max'),
    Output('page-slider-bottom', 'marks'),
    Input('interval-component', 'n_intervals'),
    Input('sort-mode', 'value'),
    Input('page-slider-top', 'value'),
    Input('page-slider-bottom', 'value')
)
def update_table(n, sort_by, page_top, page_bottom):
    current_page = page_top or page_bottom or 1
    df = load_data().sort_values(by=sort_by, ascending=False)

    per_page = 15
    total_pages = max(1, (len(df) + per_page - 1) // per_page)
    start = (current_page - 1) * per_page
    df_page = df.iloc[start:start + per_page]

    marks = {i: str(i) for i in range(1, total_pages + 1)}
    style_conditional = [
        {'if': {'filter_query': '{status} = "CRITIQUE"'}, 'backgroundColor': COLOR_CRIT, 'color': 'black'},
        {'if': {'filter_query': '{status} = "ALERTE"'}, 'backgroundColor': COLOR_WARN, 'color': 'black'},
        {'if': {'filter_query': '{status} = "NORMAL"'}, 'backgroundColor': COLOR_OK, 'color': 'black'}
    ]
    return df_page.to_dict('records'), style_conditional, total_pages, marks, total_pages, marks

@app.callback(
    Output('selected-pid', 'data'),
    Input('process-table', 'selected_rows'),
    State('process-table', 'data')
)
def select_process(selected_rows, data):
    if selected_rows:
        return data[selected_rows[0]]['pid']
    return None

@app.callback(
    Output('killed-pid-store', 'data'),
    Output('selected-process', 'children'),
    Input('terminate-button', 'n_clicks'),
    State('selected-pid', 'data'),
    State('killed-pid-store', 'data'),
    prevent_initial_call=True
)
def terminate_selected(n_clicks, pid, killed_pids):
    if pid:
        try:
            os.kill(int(pid), signal.SIGKILL)
            killed_pids.append(pid)
            return killed_pids, f"✅ Processus PID {pid} terminé."
        except Exception as e:
            return killed_pids, f"❌ Erreur lors de la terminaison du processus {pid} : {e}"
    return killed_pids, "❗ Aucun processus sélectionné."

@app.callback(
    Output('page-slider-top', 'value'),
    Output('page-slider-bottom', 'value'),
    Input('page-slider-top', 'value'),
    Input('page-slider-bottom', 'value')
)
def sync_sliders(val_top, val_bottom):
    ctx_id = ctx.triggered_id
    value = val_top if ctx_id == 'page-slider-top' else val_bottom
    return value, value

@app.callback(
    Output('export-status', 'children'),
    Input('export-button', 'n_clicks'),
    prevent_initial_call=True
)
def export_to_csv(n):
    df = load_data()
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    filename = f"moniteur_export_{timestamp}.csv"
    df.to_csv(filename, index=False)
    return f"✅ Données exportées dans le fichier : {filename}"

# === CALLBACK ORDONNANCEUR FIFO ===

@app.callback(
    Output('fifo-info', 'children'),
    Input('fifo-interval', 'n_intervals')
)
def update_fifo_info(n):
    data = load_fifo_data()
    tasks = data.get("tasks_in_queue", [])
    current = data.get("current_task")
    processed = data.get("tasks_processed", 0)
    avg_wait = data.get("average_wait_time", 0)
    throughput = data.get("throughput", 0)

    # Formatage texte multi-ligne
    text = (
        f"Tâches en file : {tasks}\n"
        f"Tâche en cours : {current}\n"
        f"Tâches traitées : {processed}\n"
        f"Temps d'attente moyen : {avg_wait:.2f} s\n"
        f"Débit (throughput) : {throughput:.2f} tâches/s"
    )
    return text

if __name__ == '__main__':
    app.run(debug=True)

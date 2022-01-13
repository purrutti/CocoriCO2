using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Net.WebSockets;
using System.Threading;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.IO;
using Newtonsoft.Json;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using LiveCharts;
using LiveCharts.Configurations;
using System.Diagnostics;
using SlackAPI;

namespace Appli_CocoriCO2
{
    public class Ambiant
    {

        [JsonProperty("sun", Required = Required.Default)]
        public bool sun { get; set; }
        [JsonProperty("tide", Required = Required.Default)]
        public bool tide { get; set; }
        [JsonProperty("oxy", Required = Required.Default)]
        public double oxy { get; set; }
        [JsonProperty("cond", Required = Required.Default)]
        public double cond { get; set; }
        [JsonProperty("turb", Required = Required.Default)]
        public double turb { get; set; }
        [JsonProperty("fluo", Required = Required.Default)]
        public double fluo { get; set; }
        [JsonProperty("temp", Required = Required.Default)]
        public double temperature { get; set; }
        [JsonProperty("sal", Required = Required.Default)]
        public double salinite { get; set; }
        [JsonProperty("pH", Required = Required.Default)]
        public double pH { get; set; }
        [JsonProperty("sPID_EA", Required = Required.Default)]
        public double sortiePID_EA { get; set; }
        [JsonProperty("sPID_EC", Required = Required.Default)]
        public double sortiePID_EC { get; set; }
        [JsonProperty("sPID_TEC", Required = Required.Default)]
        public double sortiePID_TEC { get; set; }
        [JsonProperty("tempPAC", Required = Required.Default)]
        public double tempPAC { get; set; }
        [JsonProperty("pressionEA", Required = Required.Default)]
        public double pressionEA { get; set; }
        [JsonProperty("pressionEC", Required = Required.Default)]
        public double pressionEC { get; set; }

        [JsonProperty("nextSunUp", Required = Required.Default)]
        public long nextSunUp { get; set; }
        [JsonProperty("nextSunDown", Required = Required.Default)]
        public long nextSunDown { get; set; }

        public long time { get; set; }
        public DateTime lastUpdated { get; set; }
    }
    public class Mesocosme
    {
        [JsonProperty("MesoID", Required = Required.Default)]
        public int mesocosmeID { get; set; }
        [JsonProperty("LevelH", Required = Required.Default)]
        public bool alarmeNiveauHaut { get; set; }
        [JsonProperty("LevelL", Required = Required.Default)]
        public bool alarmeNiveauBas { get; set; }
        [JsonProperty("LevelLL", Required = Required.Default)]
        public bool alarmeNiveauTresBas { get; set; }
        [JsonProperty("debit", Required = Required.Default)]
        public double debit { get; set; }
        [JsonProperty("temp", Required = Required.Default)]
        public double temperature { get; set; }
        [JsonProperty("pH", Required = Required.Default)]
        public double pH { get; set; }
    }
    public class Regul
    {
        [JsonProperty("sPID", Required = Required.Default)]
        public double sortiePID { get; set; }
        [JsonProperty("cons", Required = Required.Default)]
        public double consigne { get; set; }
        [JsonProperty("Kp", Required = Required.Default)]
        public double Kp { get; set; }
        [JsonProperty("Ki", Required = Required.Default)]
        public double Ki { get; set; }
        [JsonProperty("Kd", Required = Required.Default)]
        public double Kd { get; set; }
        [JsonProperty("sPID_pc", Required = Required.Default)]
        public double sortiePID_pc { get; set; }
        [JsonProperty("aForcage", Required = Required.Default)]
        public bool autorisationForcage { get; set; }
        [JsonProperty("consForcage", Required = Required.Default)]
        public int consigneForcage { get; set; }
        [JsonProperty("offset", Required = Required.Default)]
        public double offset { get; set; }

    }
    public class Condition
    {
        [JsonProperty("cmd", Required = Required.Default)]
        public int command { get; set; }
        [JsonProperty("temp", Required = Required.Default)]
        public double temperature { get; set; }
        [JsonProperty("pH", Required = Required.Default)]
        public double pH { get; set; }
        [JsonProperty("cID", Required = Required.Default)]
        public int condID { get; set; }
        [JsonProperty("data", Required = Required.Default)]
        public Mesocosme[] Meso { get; set; }
        [JsonProperty("rTemp", Required = Required.Default)]
        public Regul regulTemp { get; set; }
        [JsonProperty("rpH", Required = Required.Default)]
        public Regul regulpH { get; set; }
        public long time { get; set; }
        public DateTime lastUpdated { get; set; }

    }

    public class MasterParams
    {
        [JsonProperty("rPressionEA", Required = Required.Default)]
        public Regul regulPressionEA;
        [JsonProperty("rPressionEC", Required = Required.Default)]
        public Regul regulPressionEC;
        public MasterParams()
        {
            regulPressionEA = new Regul();
            regulPressionEC = new Regul();
        }
    }

    public class PACParams
    {

        [JsonProperty("rTempEC", Required = Required.Default)]
        public Regul regulTempEC;
        public PACParams()
        {
            regulTempEC = new Regul();
        }
    }

    public unsafe class Alarme
    {
        public string libelle { get; set; }
        public DateTime dtTriggered { get; set; }
        public DateTime dtRaised { get; set; }
        public DateTime dtAcknowledged { get; set; }
        public TimeSpan delay { get; set; }
        public double threshold { get; set; }
        public double delta { get; set; }
        public double value { get; set; }
        public bool enabled { get; set; }
        public bool triggered { get; set; }
        public bool raised { get; set; }
        public bool acknowledged { get; set; }
        public int comparaison { get; set; }
        public bool checkAndRaise(double val) // raise alarm if value is upperThan threshold
        {
            value = val;
            if (!enabled) return false;
            bool upperThan, lowerThan;
            switch (comparaison)
            {
                case 0:
                    upperThan = true;
                    lowerThan = false;
                    break;
                case 1:
                    upperThan = false;
                    lowerThan = true;
                    break;
                case 2:
                    upperThan = true;
                    lowerThan = true;
                    break;
                default:
                    upperThan = false;
                    lowerThan = false;
                    break;
            }


            bool t = false;
            if (triggered) t = true;
            if (!triggered && upperThan && value >= (threshold + delta))
            {
                dtTriggered = DateTime.Now;
                t = true;
            }
            if (!triggered && lowerThan && value <= (threshold - delta))
            {
                dtTriggered = DateTime.Now;
                t = true;
            }
            triggered = t;

            if (!raised && triggered && dtTriggered.Add(delay) < DateTime.Now)
            {
                raised = true;
                dtRaised = DateTime.Now;
                sendSlackMessage(this.libelle + ": Measure = " + value.ToString() + ", Set point = " + threshold.ToString() + ", triggered at:" + dtTriggered.ToString()); ;
            }
            if (raised) return true;
            return false;
        }

        private void sendSlackMessage(String msg)
        {
            string TOKEN = Properties.Settings.Default["SlackToken"].ToString();  // token from last step in section above
            var slackClient = new SlackTaskClient(TOKEN);

            slackClient.PostMessageAsync(Properties.Settings.Default["SlackChannelID"].ToString(), msg);
        }

        public bool checkAndRaise(bool val, bool th) // raise alarm if value is upperThan threshold
        {
            if (!enabled) return false;


            if (!triggered && val != th)
            {
                triggered = true;
                dtTriggered = DateTime.Now;
            }
            else triggered = false;

            if (!raised && triggered && dtTriggered.Add(delay) > DateTime.Now)
            {
                raised = true;
                dtRaised = DateTime.Now;
                sendSlackMessage(this.libelle + " triggered at:" + dtTriggered.ToString());

            }
            if (raised) return true;
            return false;
        }


        public unsafe void set(string l, bool ena, int comp, double d, TimeSpan del)
        {
            libelle = l;
            enabled = ena;
            comparaison = comp;
            delta = d;
            delay = del;
        }
    }


    /// <summary>
    /// Logique d'interaction pour MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        //public List<Condition> conditions;
        public ObservableCollection<Condition> conditions;
        public ObservableCollection<Condition> conditionData;
        public ObservableCollection<Alarme> alarms;
        public Ambiant ambiantConditions = new Ambiant();

        //public List<Condition> conditionData;
        public ExpSettingsWindow expSettingsWindow;
        public AlarmsListWindow alarmsListWindow;
        public ComDebugWindow comDebugWindow;
        public Calibration calibrationWindow;
        public CultureInfo ci;

        public ClientWebSocket ws = new ClientWebSocket();
#pragma warning disable CS0414 // Le champ 'MainWindow.autoReco' est assigné, mais sa valeur n'est jamais utilisée
        bool autoReco;
#pragma warning restore CS0414 // Le champ 'MainWindow.autoReco' est assigné, mais sa valeur n'est jamais utilisée
        public int step;

        public bool cleanupMode;

        public MasterParams masterParams;
        public PACParams pacParams;

        public string[] Labels = new[] { "0" };
        public MainWindow()
        {
            if (Process.GetProcessesByName(Process.GetCurrentProcess().ProcessName).Length > 1)
            {
                MessageBox.Show("CocoriCO2 Application is already running. Only one instance of this application is allowed");
                System.Windows.Application.Current.Shutdown();
            }
            else
            {
                InitializeComponent();
                var cts = new CancellationTokenSource();
                cleanupMode = false;


#pragma warning disable CS4014 // Dans la mesure où cet appel n'est pas attendu, l'exécution de la méthode actuelle continue avant la fin de l'appel. Envisagez d'appliquer l'opérateur 'await' au résultat de l'appel.

#pragma warning restore CS4014 // Dans la mesure où cet appel n'est pas attendu, l'exécution de la méthode actuelle continue avant la fin de l'appel. Envisagez d'appliquer l'opérateur 'await' au résultat de l'appel.
                autoReco = false;
                conditions = new ObservableCollection<Condition>();
                conditionData = new ObservableCollection<Condition>();

                alarms = new ObservableCollection<Alarme>();
                for (int i = 0; i < 4; i++)
                {
                    Condition c = new Condition();
                    c.condID = i;
                    c.regulpH = new Regul();
                    c.regulTemp = new Regul();
                    c.Meso = new Mesocosme[3];
                    for (int j = 0; j < 3; j++) c.Meso[j] = new Mesocosme();
                    c.temperature = i + 29.99;
                    conditions.Add(c);
                }



                expSettingsWindow = new ExpSettingsWindow();
                comDebugWindow = new ComDebugWindow();
                calibrationWindow = new Calibration();


                InitializeAsync();
                InitializeAsyncAlarms();

                masterParams = new MasterParams();
                pacParams = new PACParams();

                ci = new CultureInfo("en-US");
                ci.NumberFormat.NumberDecimalDigits = 2;
                ci.NumberFormat.NumberDecimalSeparator = ".";
                ci.NumberFormat.NumberGroupSeparator = " ";
                Thread.CurrentThread.CurrentCulture = ci;
                Thread.CurrentThread.CurrentUICulture = ci;
                CultureInfo.DefaultThreadCurrentCulture = ci;
                CultureInfo.DefaultThreadCurrentUICulture = ci;

                setAlarms();

                alarmsListWindow = new AlarmsListWindow();
            }


        }

        private void checkAlarme(string libelle, double value, double t)
        {
            try
            {
                Alarme a = alarms.Single(alarm => alarm.libelle == libelle);
                a.threshold = t;
                a.checkAndRaise(value);
            }
            catch (Exception e)
            {

            }

        }

        private void checkAlarme(string libelle, bool value, bool threshold)
        {


            try
            {
                Alarme a = alarms.Single(alarm => alarm.libelle == libelle);
                a.checkAndRaise(value, threshold);
            }
            catch (Exception e)
            {

            }
        }

        private void checkAlarms()
        {
            if (cleanupMode) return;
            double d;

            string cond, meso;
            checkAlarme("Alarm Pressure Ambient water", ambiantConditions.pressionEA, masterParams.regulPressionEA.consigne);
            checkAlarme("AlarmPressure Hot Water", ambiantConditions.pressionEC, masterParams.regulPressionEC.consigne);


            cond = "C0";

            checkAlarme(cond + " Mixing Tank: Alarm pH", conditions[0].pH, conditions[0].regulpH.consigne);




            for (int j = 0; j < 3; j++)//mesocosmes
            {
                meso = "M" + j;


                if (ambiantConditions.tide)//vanne exondation ouverte
                    checkAlarme(cond + meso + ": Alarm Low level", conditions[0].Meso[j].alarmeNiveauBas, false);
                else checkAlarme(cond + meso + ": Alarm Low Level", conditions[0].Meso[j].alarmeNiveauBas, true);



                checkAlarme(cond + meso + ": Alarm Very Low Level", conditions[0].Meso[j].alarmeNiveauTresBas, false);

                Double.TryParse(Properties.Settings.Default["FlowrateSetpoint"].ToString(), out d);

                checkAlarme(cond + meso + ": Alarm Flowrate", conditions[0].Meso[j].debit, d);
                checkAlarme(cond + meso + ": Alarm pH", conditions[0].Meso[j].pH, ambiantConditions.pH);
                checkAlarme(cond + meso + ": Alarm Temperature", conditions[0].Meso[j].temperature, ambiantConditions.temperature);

            }


            for (int i = 1; i < 4; i++) //conditions
            {
                cond = "C" + i;

                checkAlarme(cond + " Mixing Tank: Alarm pH", conditions[i].pH, conditions[i].regulpH.consigne);
                checkAlarme(cond + " Mixing Tank: Alarm Temperature", conditions[i].temperature, conditions[i].regulTemp.consigne);




                for (int j = 0; j < 3; j++)//mesocosmes
                {
                    meso = "M" + j;

                    checkAlarme(cond + meso + ": Alarm Overflood", conditions[i].Meso[j].alarmeNiveauHaut, false);

                    if (ambiantConditions.tide)//vanne exondation ouverte
                        checkAlarme(cond + meso + ": Alarm Low Level", conditions[i].Meso[j].alarmeNiveauBas, false);
                    else checkAlarme(cond + meso + ": Alarm Low Level", conditions[i].Meso[j].alarmeNiveauBas, true);



                    checkAlarme(cond + meso + ": Alarm Very Low Level", conditions[i].Meso[j].alarmeNiveauTresBas, false);

                    Double.TryParse(Properties.Settings.Default["FlowrateSetpoint"].ToString(), out d);

                    checkAlarme(cond + meso + ": Alarm Flowrate", conditions[i].Meso[j].debit, d);
                    checkAlarme(cond + meso + ": Alarm pH", conditions[i].Meso[j].pH, conditions[i].regulpH.consigne);
                    checkAlarme(cond + meso + ": Alarm Temperature", conditions[i].Meso[j].temperature, conditions[i].regulTemp.consigne);

                }
            }

            if (alarmsListWindow._lastHeaderClicked != null) alarmsListWindow.Sort(alarmsListWindow._lastHeaderClicked.Tag as string, alarmsListWindow._lastDirection);
            else alarmsListWindow.Sort("raised", alarmsListWindow._lastDirection);
        }

        public unsafe void setAlarms()
        {
            alarms.Clear();


            string cond;
            string meso;
            bool e;
            double d;


            cond = "C0";
            Boolean.TryParse(Properties.Settings.Default["AlarmPressure"].ToString(), out e);
            Double.TryParse(Properties.Settings.Default["PressureDelta"].ToString(), out d);
            Alarme a = new Alarme();
            a.set("Alarm Pressure Ambient water", e, 2, d, TimeSpan.FromSeconds(30));
            alarms.Add(a);
            Alarme b = new Alarme();
            b.set("Alarm Pressure Hot water", e, 2, d, TimeSpan.FromSeconds(30));
            alarms.Add(b);

            for (int i = 0; i < 4; i++) //conditions
            {
                cond = "C" + i;


                Double.TryParse(Properties.Settings.Default["ConditionpHDelta"].ToString(), out d);
                Boolean.TryParse(Properties.Settings.Default["AlarmpHCondition"].ToString(), out e);
                Alarme c = new Alarme();
                c.set(cond + " Mixing Tank: Alarm pH", e, 2, d, TimeSpan.FromSeconds(30));
                alarms.Add(c);

                if (i > 0)
                {
                    Double.TryParse(Properties.Settings.Default["ConditionTempDelta"].ToString(), out d);
                    Boolean.TryParse(Properties.Settings.Default["AlarmTempCondition"].ToString(), out e);
                    Alarme f = new Alarme();
                    f.set(cond + " Mixing Tank: Alarm Temperature", e, 2, d, TimeSpan.FromSeconds(30));
                    alarms.Add(f);
                }

                for (int j = 0; j < 3; j++)//mesocosmes
                {
                    meso = "M" + j;
                    Boolean.TryParse(Properties.Settings.Default["AlarmLevelH"].ToString(), out e);
                    Alarme g = new Alarme();
                    g.set(cond + meso + ": Alarm Overflood", e, 0, 0, TimeSpan.FromSeconds(30));
                    alarms.Add(g);
                    Boolean.TryParse(Properties.Settings.Default["AlarmLevelL"].ToString(), out e);
                    Alarme h = new Alarme();
                    h.set(cond + meso + ": Alarm Low Level", e, 0, 0, TimeSpan.FromMinutes(90));
                    alarms.Add(h);
                    Boolean.TryParse(Properties.Settings.Default["AlarmLevelLL"].ToString(), out e);
                    Alarme k = new Alarme();
                    k.set(cond + meso + ": Alarm Very Low Level", e, 0, 0, TimeSpan.FromSeconds(30));
                    alarms.Add(k);

                    Double.TryParse(Properties.Settings.Default["FlowrateDelta"].ToString(), out d);
                    Boolean.TryParse(Properties.Settings.Default["AlarmFlowrate"].ToString(), out e);

                    Alarme l = new Alarme();
                    l.set(cond + meso + ": Alarm Flowrate", e, 2, d, TimeSpan.FromSeconds(30));
                    alarms.Add(l);

                    Double.TryParse(Properties.Settings.Default["MesocosmpHDelta"].ToString(), out d);
                    Boolean.TryParse(Properties.Settings.Default["AlarmpHMesocosm"].ToString(), out e);
                    Alarme m = new Alarme();
                    m.set(cond + meso + ": Alarm pH", e, 2, d, TimeSpan.FromSeconds(30));
                    alarms.Add(m);
                    Double.TryParse(Properties.Settings.Default["MesocosmTempDelta"].ToString(), out d);
                    Boolean.TryParse(Properties.Settings.Default["AlarmtempMesocosm"].ToString(), out e);
                    Alarme n = new Alarme();
                    n.set(cond + meso + ": Alarm Temperature", e, 2, d, TimeSpan.FromSeconds(30));
                    alarms.Add(n);

                }
            }
        }

        private void checkConnection()
        {
            switch (ws.State)
            {
                case WebSocketState.Open:
                    Connect_btn.Header = "Disconnect";
                    Connect_btn.IsEnabled = true;
                    statusLabel.Text = "Connection Status: Connected";

                    sendRequest();
                    break;
                case WebSocketState.Closed:
                    Connect_btn.Header = "Connect";
                    Connect_btn.IsEnabled = true;
                    statusLabel.Text = "Connection Status: Disconnected (Closed)";
                    ws = new ClientWebSocket();
                    Connect();
                    break;
                case WebSocketState.Aborted:
                    ws.Dispose();
                    ws = new ClientWebSocket();
                    Connect_btn.Header = "Connect";
                    Connect_btn.IsEnabled = true;
                    statusLabel.Text = "Connection Status: Disconnected (Aborted)";
                    Connect();
                    break;
                case WebSocketState.None:
                    Connect_btn.Header = "Connect";
                    Connect_btn.IsEnabled = true;
                    statusLabel.Text = "Connection Status: Disconnected (None)";
                    Connect();
                    break;
                case WebSocketState.Connecting:
                    Connect_btn.Header = "Connecting";
                    Connect_btn.IsEnabled = false;
                    statusLabel.Text = "Connection Status: Connecting";
                    break;
            }

        }

        private void sendRequest()
        {
            string msg = "";

            var Timestamp = new DateTimeOffset(DateTime.UtcNow).ToUnixTimeSeconds();
            if (conditions[0].regulpH.consigne == 0)
            {
                for (int i = 0; i < 6; i++) expSettingsWindow.load(i);
            }
            else
            {
                switch (step)
                {
                    case 0:
                        msg = "{\"cmd\":1,\"cID\":0,\"sID\":4}";
                        break;
                    case 1:
                        msg = "{\"cmd\":1,\"cID\":1,\"sID\":4}";
                        break;
                    case 2:
                        msg = "{\"cmd\":1,\"cID\":2,\"sID\":4}";
                        break;
                    case 3:
                        msg = "{\"cmd\":1,\"cID\":3,\"sID\":4}";
                        break;
                    case 4:
                        msg = "{\"cmd\":5,\"cID\":0,\"sID\":4,\"time\":" + Timestamp + "}";
                        break;
                }
                if (step < 4) step++; else step = 0;

                comDebugWindow.tb1.Text = msg;

                Task<string> t2 = Send(ws, msg, comDebugWindow.tb2);
                t2.Wait(20);
            }

        }





        private static async Task<string> Send(ClientWebSocket ws, string msg, TextBox tb)
        {

            var timeOut = new CancellationTokenSource(500).Token;
            if (ws.State == WebSocketState.Open)
            {
                ArraySegment<byte> bytesToSend = new ArraySegment<byte>(
                    Encoding.UTF8.GetBytes(msg));
                await ws.SendAsync(
                    bytesToSend, WebSocketMessageType.Text,
                    true, timeOut);

                ArraySegment<byte> bytesReceived = new ArraySegment<byte>(new byte[1024]);
                WebSocketReceiveResult result = ws.ReceiveAsync(
                    bytesReceived, timeOut).Result;
                string data = Encoding.UTF8.GetString(bytesReceived.Array, 0, result.Count);
                tb.Text = data;
                return data;
            }
            return null;

        }



        public void DisplayData(int command)
        {
            label_time.Content = DateTime.Now.ToUniversalTime();
            Cleanup_btn.Header = cleanupMode ? "Exit Cleanup Mode" : "Start Cleanup Mode";

            statusLabel2.Text = cleanupMode ? "Cleanup Mode Active" : "";
            statusLabel2.Foreground = cleanupMode ? new SolidColorBrush(Colors.Red) : new SolidColorBrush(Colors.Black);


            if (expSettingsWindow.ShowActivated)
            {
                int selctedcondID = expSettingsWindow.comboBox_Condition.SelectedIndex;
                if (selctedcondID == 4)
                {
                    expSettingsWindow.tb_pH_measure.Text = ambiantConditions.pressionEA.ToString(ci);
                    expSettingsWindow.tb_pH_PIDoutput.Text = ambiantConditions.sortiePID_EA.ToString(ci);
                    expSettingsWindow.tb_Temp_measure.Text = ambiantConditions.pressionEC.ToString(ci);
                    expSettingsWindow.tb_Temp_PIDoutput.Text = ambiantConditions.sortiePID_EC.ToString(ci);
                }
                else
                if (selctedcondID == 5)
                {
                    expSettingsWindow.tb_pH_measure.Text = ambiantConditions.tempPAC.ToString(ci);
                    expSettingsWindow.tb_pH_setPoint.Text = (ambiantConditions.temperature + pacParams.regulTempEC.offset).ToString(ci);
                    expSettingsWindow.tb_pH_PIDoutput.Text = ambiantConditions.sortiePID_TEC.ToString(ci);
                }
                else
                {
                    if (selctedcondID < 0 || selctedcondID > 3) selctedcondID = 0;
                    expSettingsWindow.tb_pH_measure.Text = conditions[selctedcondID].pH.ToString(ci);
                    expSettingsWindow.tb_pH_PIDoutput.Text = conditions[selctedcondID].regulpH.sortiePID_pc.ToString(ci);
                    expSettingsWindow.tb_Temp_measure.Text = conditions[selctedcondID].temperature.ToString(ci);
                    expSettingsWindow.tb_Temp_PIDoutput.Text = conditions[selctedcondID].regulTemp.sortiePID_pc.ToString(ci);
                }
            }


            //expSettingsWindow.tb_pH_setPoint.Text = conditions[0].regulpH.consigne.ToString();
            if (command == 2)//PARAMS
            {

            }
            else if (command == 3)//DATA
            {
                label_C0M1_Alarm.Content = "";
                label_C0M2_Alarm.Content = "";
                label_C0M3_Alarm.Content = "";
                label_C1M1_Alarm.Content = "";
                label_C1M2_Alarm.Content = "";
                label_C1M3_Alarm.Content = "";
                label_C2M1_Alarm.Content = "";
                label_C2M2_Alarm.Content = "";
                label_C2M3_Alarm.Content = "";
                label_C3M1_Alarm.Content = "";
                label_C3M2_Alarm.Content = "";
                label_C3M3_Alarm.Content = "";
                if (ambiantConditions.tide)//vanne exondation ouverte
                {
                    if (conditions[0].Meso[0].alarmeNiveauBas) label_C0M1_Alarm.Content = "Alarm: Exondation not effective";
                }
                else if (conditions[0].Meso[0].alarmeNiveauBas) label_C0M1_Alarm.Content = "Alarm: Low level";
                else label_C0M1_Alarm.Content = !conditions[0].Meso[0].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";



                if (ambiantConditions.tide)//vanne exondation ouverte
                {
                    if (conditions[0].Meso[1].alarmeNiveauBas) label_C0M2_Alarm.Content = "Alarm: Exondation not effective";
                }
                else if (conditions[0].Meso[1].alarmeNiveauBas) label_C0M2_Alarm.Content = "Alarm: Low level";
                else label_C0M2_Alarm.Content = !conditions[0].Meso[1].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";


                if (ambiantConditions.tide)//vanne exondation ouverte
                {
                    if (conditions[0].Meso[2].alarmeNiveauBas) label_C0M3_Alarm.Content = "Alarm: Exondation not effective";
                }
                else if (conditions[0].Meso[2].alarmeNiveauBas) label_C0M3_Alarm.Content = "Alarm: Low level";
                else label_C0M3_Alarm.Content = !conditions[0].Meso[2].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";


                if (conditions[1].Meso[0].alarmeNiveauHaut)
                {
                    label_C1M1_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[1].Meso[0].alarmeNiveauBas) label_C1M1_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else if (conditions[1].Meso[0].alarmeNiveauBas) label_C1M1_Alarm.Content = "Alarm: Low level";
                    else label_C1M1_Alarm.Content = !conditions[1].Meso[0].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }

                if (conditions[1].Meso[1].alarmeNiveauHaut)
                {
                    label_C1M2_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[1].Meso[1].alarmeNiveauBas) label_C1M2_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else if (conditions[1].Meso[1].alarmeNiveauBas) label_C1M2_Alarm.Content = "Alarm: Low level";
                    else label_C1M2_Alarm.Content = !conditions[1].Meso[1].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }
                if (conditions[1].Meso[2].alarmeNiveauHaut)
                {
                    label_C1M3_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[1].Meso[2].alarmeNiveauBas) label_C1M3_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else if (conditions[1].Meso[2].alarmeNiveauBas) label_C1M3_Alarm.Content = "Alarm: Low level";
                    else label_C1M3_Alarm.Content = !conditions[1].Meso[2].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }
                if (conditions[2].Meso[0].alarmeNiveauHaut)
                {
                    label_C2M1_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[2].Meso[0].alarmeNiveauBas) label_C2M1_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else if (conditions[2].Meso[0].alarmeNiveauBas) label_C2M1_Alarm.Content = "Alarm: Low level";
                    else label_C2M1_Alarm.Content = !conditions[2].Meso[0].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }
                if (conditions[2].Meso[1].alarmeNiveauHaut)
                {
                    label_C2M2_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[2].Meso[1].alarmeNiveauBas) label_C2M2_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else if (conditions[2].Meso[1].alarmeNiveauBas) label_C2M2_Alarm.Content = "Alarm: Low level";
                    else
                        label_C2M2_Alarm.Content = !conditions[2].Meso[1].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }
                if (conditions[2].Meso[2].alarmeNiveauHaut)
                {
                    label_C2M3_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[2].Meso[2].alarmeNiveauBas) label_C2M3_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else if (conditions[2].Meso[2].alarmeNiveauBas) label_C2M3_Alarm.Content = "Alarm: Low level";
                    else label_C2M3_Alarm.Content = !conditions[2].Meso[2].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }

                if (conditions[3].Meso[0].alarmeNiveauHaut)
                {
                    label_C3M1_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[3].Meso[0].alarmeNiveauBas) label_C3M1_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else if (conditions[3].Meso[0].alarmeNiveauBas) label_C3M1_Alarm.Content = "Alarm: Low level";
                    else label_C3M1_Alarm.Content = !conditions[3].Meso[0].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }
                if (conditions[3].Meso[1].alarmeNiveauHaut)
                {
                    label_C3M2_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[3].Meso[1].alarmeNiveauBas) label_C3M2_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else if (conditions[3].Meso[1].alarmeNiveauBas) label_C3M2_Alarm.Content = "Alarm: Low level";
                    else label_C3M2_Alarm.Content = !conditions[3].Meso[1].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }
                if (conditions[3].Meso[2].alarmeNiveauHaut)
                {
                    label_C3M3_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[3].Meso[2].alarmeNiveauBas) label_C3M3_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else if (conditions[3].Meso[2].alarmeNiveauBas) label_C3M3_Alarm.Content = "Alarm: Low level";
                    else label_C3M3_Alarm.Content = !conditions[3].Meso[2].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }

                label_EC_temperature_setpoint.Content = string.Format(ci, "Temperature setpoint: \t{0:0.00}°C", pacParams.regulTempEC.consigne);

                label_EA_pressure_measure.Content = string.Format(ci, "Pressure measure: {0:0.00} bars", ambiantConditions.pressionEA);
                label_EA_pressure_setpoint.Content = string.Format(ci, "Pressure setpoint: {0:0.00} bars", masterParams.regulPressionEA.consigne);
                label_EA_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", ambiantConditions.sortiePID_EA);
                label_EC_pressure_measure.Content = string.Format(ci, "Pressure measure: {0:0.00} bars", ambiantConditions.pressionEC);
                label_EC_pressure_setpoint.Content = string.Format(ci, "Pressure setpoint: {0:0.00} bars", masterParams.regulPressionEC.consigne);
                label_EC_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", ambiantConditions.sortiePID_EC);

                label_C0_pH_setpoint.Content = string.Format(ci, "pH setpoint: {0:0.00}", conditions[0].regulpH.consigne);
                label_C0_pH_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[0].regulpH.sortiePID_pc);

                label_C1_pH_setpoint.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[1].regulpH.consigne);
                label_C1_pH_sortiePID.Content = string.Format(ci, "Pump: \t{0:0}%", conditions[1].regulpH.sortiePID_pc);
                label_C1_Temp_setpoint.Content = string.Format(ci, "T°C: \t{0:0.00}", conditions[1].regulTemp.consigne);
                label_C1_Temp_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[1].regulTemp.sortiePID_pc);

                label_C2_pH_setpoint.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[2].regulpH.consigne);
                label_C2_pH_sortiePID.Content = string.Format(ci, "Pump: \t{0:0}%", conditions[2].regulpH.sortiePID_pc);
                label_C2_Temp_setpoint.Content = string.Format(ci, "T°C: \t{0:0.00}", conditions[2].regulTemp.consigne);
                label_C2_Temp_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[2].regulTemp.sortiePID_pc);

                label_C3_pH_setpoint.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[3].regulpH.consigne);
                label_C3_pH_sortiePID.Content = string.Format(ci, "Pump: \t{0:0}%", conditions[3].regulpH.sortiePID_pc);
                label_C3_Temp_setpoint.Content = string.Format(ci, "T°C: \t{0:0.00}", conditions[3].regulTemp.consigne);
                label_C3_Temp_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[3].regulTemp.sortiePID_pc);

                if (conditions[0].regulpH.autorisationForcage)
                {
                    label_C0_pH_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[0].regulpH.consigneForcage);
                    label_C0_pH_sortiePID.Foreground = Brushes.Red;
                }
                else label_C0_pH_sortiePID.Foreground = Brushes.Black;
                if (masterParams.regulPressionEA.autorisationForcage)
                {
                    label_EA_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", masterParams.regulPressionEA.consigneForcage);
                    label_EA_sortiePID.Foreground = Brushes.Red;
                }
                else label_EA_sortiePID.Foreground = Brushes.Black;
                
                if (masterParams.regulPressionEC.autorisationForcage)
                {
                    label_EC_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", masterParams.regulPressionEC.consigneForcage);
                    label_EC_sortiePID.Foreground = Brushes.Red;
                }
                else label_EC_sortiePID.Foreground = Brushes.Black;

                if (conditions[1].regulpH.autorisationForcage)
                {
                    label_C1_pH_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[1].regulpH.consigneForcage);
                    label_C1_pH_sortiePID.Foreground = Brushes.Red;
                }
                else label_C1_pH_sortiePID.Foreground = Brushes.Black;
                if (conditions[1].regulTemp.autorisationForcage)
                {
                    label_C1_Temp_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[1].regulTemp.consigneForcage);
                    label_C1_Temp_sortiePID.Foreground = Brushes.Red;
                }
                else label_C1_Temp_sortiePID.Foreground = Brushes.Black;

                if (conditions[2].regulpH.autorisationForcage)
                {
                    label_C2_pH_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[2].regulpH.consigneForcage);
                    label_C2_pH_sortiePID.Foreground = Brushes.Red;
                }
                else label_C2_pH_sortiePID.Foreground = Brushes.Black;
                if (conditions[2].regulTemp.autorisationForcage)
                {
                    label_C2_Temp_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[2].regulTemp.consigneForcage);
                    label_C2_Temp_sortiePID.Foreground = Brushes.Red;
                }
                else label_C2_Temp_sortiePID.Foreground = Brushes.Black;

                if (conditions[3].regulpH.autorisationForcage)
                {
                    label_C3_pH_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[3].regulpH.consigneForcage);
                    label_C3_pH_sortiePID.Foreground = Brushes.Red;
                }
                else label_C3_pH_sortiePID.Foreground = Brushes.Black;
                if (conditions[3].regulTemp.autorisationForcage)
                {
                    label_C3_Temp_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", conditions[3].regulTemp.consigneForcage);
                    label_C3_Temp_sortiePID.Foreground = Brushes.Red;
                }
                else label_C3_Temp_sortiePID.Foreground = Brushes.Black;


                label_C0_pH_CO2.Content = string.Format(ci, "pH measure: {0:0.00}", conditions[0].pH);
                // label_C0_Temp.Content = string.Format(ci, "T°C: {0:0.00}°C", conditions[0].temperature);
                label_C1_pH.Content = string.Format(ci, "pH: {0:0.00}", conditions[1].pH);
                label_C1_Temp.Content = string.Format(ci, "T°C: {0:0.00}°C", conditions[1].temperature);
                label_C2_pH.Content = string.Format(ci, "pH: {0:0.00}", conditions[2].pH);
                label_C2_Temp.Content = string.Format(ci, "T°C: {0:0.00}°C", conditions[2].temperature);
                label_C3_pH.Content = string.Format(ci, "pH: {0:0.00}", conditions[3].pH);
                label_C3_Temp.Content = string.Format(ci, "T°C: {0:0.00}°C", conditions[3].temperature);

                label_C0M1_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[0].Meso[0].debit);
                label_C0M2_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[0].Meso[1].debit);
                label_C0M3_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[0].Meso[2].debit);
                label_C0M1_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[0].Meso[0].pH);
                label_C0M2_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[0].Meso[1].pH);
                label_C0M3_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[0].Meso[2].pH);
                label_C0M1_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[0].Meso[0].temperature);
                label_C0M2_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[0].Meso[1].temperature);
                label_C0M3_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[0].Meso[2].temperature);

                label_C1M1_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[1].Meso[0].debit);
                label_C1M2_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[1].Meso[1].debit);
                label_C1M3_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[1].Meso[2].debit);
                label_C1M1_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[1].Meso[0].pH);
                label_C1M2_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[1].Meso[1].pH);
                label_C1M3_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[1].Meso[2].pH);
                label_C1M1_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[1].Meso[0].temperature);
                label_C1M2_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[1].Meso[1].temperature);
                label_C1M3_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[1].Meso[2].temperature);

                label_C2M1_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[2].Meso[0].debit);
                label_C2M2_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[2].Meso[1].debit);
                label_C2M3_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[2].Meso[2].debit);
                label_C2M1_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[2].Meso[0].pH);
                label_C2M2_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[2].Meso[1].pH);
                label_C2M3_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[2].Meso[2].pH);
                label_C2M1_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[2].Meso[0].temperature);
                label_C2M2_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[2].Meso[1].temperature);
                label_C2M3_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[2].Meso[2].temperature);

                label_C3M1_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[3].Meso[0].debit);
                label_C3M2_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[3].Meso[1].debit);
                label_C3M3_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/mn", conditions[3].Meso[2].debit);
                label_C3M1_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[3].Meso[0].pH);
                label_C3M2_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[3].Meso[1].pH);
                label_C3M3_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[3].Meso[2].pH);
                label_C3M1_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[3].Meso[0].temperature);
                label_C3M2_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[3].Meso[1].temperature);
                label_C3M3_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[3].Meso[2].temperature);
            }
            else if (command == 6)//MASTER DATA
            {
                label_C0_02.Content = string.Format(ci, "O₂: \t{0:0.00}%", ambiantConditions.oxy);
                label_C0_Salinity.Content = string.Format(ci, "Salinity: \t{0:0.00}", ambiantConditions.salinite);
                label_C0_Turb.Content = string.Format(ci, "Turb.: \t{0:0.00}", ambiantConditions.turb);
                label_C0_Fluo.Content = string.Format(ci, "Fluo.: \t{0:0.00} µg/L", ambiantConditions.fluo);
                label_C0_Temp.Content = string.Format(ci, "Temp.: \t{0:0.00}°C", ambiantConditions.temperature);
                label_C0_pH.Content = string.Format(ci, "pH: \t{0:0.00}", ambiantConditions.pH);
                if (ambiantConditions.sun) label_ledstate.Content = string.Format(ci, "LED state: ON (Day)");
                else label_ledstate.Content = string.Format(ci, "LED state: OFF (Night)");

                label_EC_temperature_measure.Content = string.Format(ci, "Temperature measure: \t{0:0.00}°C", ambiantConditions.tempPAC);
                if (pacParams.regulTempEC.autorisationForcage)
                {
                    label_TEC_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", pacParams.regulTempEC.consigneForcage);
                    label_TEC_sortiePID.Foreground = Brushes.Red;
                }
                else
                {
                    label_TEC_sortiePID.Content = string.Format(ci, "Valve: \t{0:0}%", ambiantConditions.sortiePID_TEC);
                    label_TEC_sortiePID.Foreground = Brushes.Black;
                }

                DateTime dt = new DateTime(1970, 1, 1, 0, 0, 0, 0, System.DateTimeKind.Utc).AddSeconds(ambiantConditions.nextSunDown).ToUniversalTime();
                label_nextSunDown.Content = "Sunset time: " + dt.ToString();
                dt = new DateTime(1970, 1, 1, 0, 0, 0, 0, System.DateTimeKind.Utc).AddSeconds(ambiantConditions.nextSunUp).ToUniversalTime();
                label_nextSunUp.Content = "Sunrise time: " + dt.ToString();
            }
        }



        private static async Task Connect(ClientWebSocket ws, TextBox tb)
        {
            CancellationTokenSource cts = new CancellationTokenSource();
            cts.Token.ThrowIfCancellationRequested();
            string address = Properties.Settings.Default["MasterIPAddress"].ToString();
            Uri serverUri = new Uri("ws://" + address + ":81");

            try
            {
                await ws.ConnectAsync(serverUri, cts.Token);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }


            if (ws.State == WebSocketState.Open)
            {
                CancellationTokenSource cts1 = new CancellationTokenSource();
                cts1.Token.ThrowIfCancellationRequested();
                ArraySegment<byte> bytesReceived = new ArraySegment<byte>(new byte[1024]);

                try
                {
                    WebSocketReceiveResult result = await ws.ReceiveAsync(
                    bytesReceived, cts1.Token);
                    tb.Text = Encoding.UTF8.GetString(bytesReceived.Array, 0, result.Count);
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.Message);
                }
            }

        }


        private void Connect()
        {

            if (ws.State != WebSocketState.Open)
            {
                Task t = Connect(ws, comDebugWindow.tb2);
                t.Wait(50);
                Connect_btn.Header = "Connecting";
                Connect_btn.IsEnabled = false;

            }
        }



        private static async Task RunPeriodicAsync(Action onTick,
                                           TimeSpan dueTime,
                                           TimeSpan interval,
                                           CancellationToken token)
        {
            // Initial wait time before we begin the periodic loop.
            if (dueTime > TimeSpan.Zero)
                await Task.Delay(dueTime, token);

            // Repeat this loop until cancelled.
            while (true)
            {
                // Call our onTick function.
                onTick?.Invoke();

                // Wait to repeat again.
                if (interval > TimeSpan.Zero)
                    await Task.Delay(interval, CancellationToken.None);
            }
        }

        private async Task InitializeAsync()
        {
            int t;
            Int32.TryParse(Properties.Settings.Default["dataQueryInterval"].ToString(), out t);
            var dueTime = TimeSpan.FromSeconds(0);
            var interval = TimeSpan.FromSeconds(t);

            var cancel = new CancellationTokenSource();
            cancel.Token.ThrowIfCancellationRequested();

            // TODO: Add a CancellationTokenSource and supply the token here instead of None.
            try
            {

                await RunPeriodicAsync(checkConnection, dueTime, interval, cancel.Token);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                await InitializeAsync();
            }
        }

        private async Task InitializeAsyncAlarms()
        {

            var dueTime = TimeSpan.FromSeconds(10);
            var interval = TimeSpan.FromSeconds(5);

            var cancel = new CancellationTokenSource();
            cancel.Token.ThrowIfCancellationRequested();

            // TODO: Add a CancellationTokenSource and supply the token here instead of None.
            try
            {

                await RunPeriodicAsync(checkAlarms, dueTime, interval, cancel.Token);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                await InitializeAsyncAlarms();
            }
        }



        private void Connect_Click(object sender, RoutedEventArgs e)
        {
            switch (ws.State)
            {
                case WebSocketState.Open:
                    try
                    {
                        ws.CloseAsync(WebSocketCloseStatus.NormalClosure, "Done", CancellationToken.None);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message);
                    }
                    break;
                case WebSocketState.Closed:
                case WebSocketState.Aborted:
                    ws.Dispose();
                    ws = new ClientWebSocket();
                    Connect();
                    break;
                case WebSocketState.None:
                    Connect();
                    break;
            }
        }

        private void Exit_Click(object sender, RoutedEventArgs e)
        {
            CancelEventArgs ce = new CancelEventArgs();
            Window_Closing(sender, ce);
        }

        private void AppSettings_Click(object sender, RoutedEventArgs e)
        {
            AppSettingsWindow appSettingsWindow = new AppSettingsWindow();
            appSettingsWindow.Show();
        }

        private void ExpSettings_Click(object sender, RoutedEventArgs e)
        {
            for (int i = 0; i < 6; i++) expSettingsWindow.load(i);
            expSettingsWindow.Show();
            expSettingsWindow.Focus();
        }

        private void Calibrate_btn_Click(object sender, RoutedEventArgs e)
        {
            calibrationWindow.Show();
            calibrationWindow.Focus();
        }



        private void ComDebug_Click(object sender, RoutedEventArgs e)
        {
            comDebugWindow.Show();
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            Properties.Settings.Default.Save();
            comDebugWindow.Close();
            expSettingsWindow.Close();
            alarmsListWindow.Close();
            System.Windows.Application.Current.Shutdown();
        }

        private void Ellipse_MouseDown_1(object sender, MouseButtonEventArgs e)
        {

        }

        private void Ellipse_MouseDown_2(object sender, MouseButtonEventArgs e)
        {

        }

        private void Monitoring_btn_Click(object sender, RoutedEventArgs e)
        {
            System.Diagnostics.Process.Start(Properties.Settings.Default["InfluxDBWebpage"].ToString());
        }

        private void RData_btn_Click(object sender, RoutedEventArgs e)
        {
            System.Diagnostics.Process.Start(Properties.Settings.Default["RDataWebpage"].ToString());
        }

        private void AlarmsSettings_Click(object sender, RoutedEventArgs e)
        {
            Alarms alarmsWindow = new Alarms();
            alarmsWindow.Show();
        }

        private void AlarmsList_Click(object sender, RoutedEventArgs e)
        {
            alarmsListWindow.Show();
            alarmsListWindow.Focus();
        }

        private void CleanUp_Click(object sender, RoutedEventArgs e)
        {
            if (!cleanupMode)
            {
                if (MessageBox.Show("Are you sure you want to switch to Cleanup mode?", "Cleanup Mode", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
                {
                    cleanupMode = !cleanupMode;
                }
            }
            else
            {
                if (MessageBox.Show("Are you sure you want to exit Cleanup mode?", "Cleanup Mode", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
                {
                    cleanupMode = !cleanupMode;
                }
            }

        }
    }
}

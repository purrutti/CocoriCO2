﻿using System;
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

namespace Appli_CocoriCO2
{
    public class Ambiant
    {

        [JsonProperty(Required = Required.Default)]
        public bool sun { get; set; }
        [JsonProperty(Required = Required.Default)]
        public bool tide { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double oxy { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double cond { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double turb { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double fluo { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double temperature { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double pH { get; set; }
        public long time { get; set; }
        public DateTime lastUpdated { get; set; }
    }
    public class Mesocosme
    {
        [JsonProperty("MesoID",Required = Required.Default)]
        public int mesocosmeID{ get; set; }
        [JsonProperty("LevelH", Required = Required.Default)]
        public bool alarmeNiveauHaut { get; set; }
        [JsonProperty("LevelL", Required = Required.Default)]
        public bool alarmeNiveauBas { get; set; }
        [JsonProperty("LevelLL", Required = Required.Default)]
        public bool alarmeNiveauTresBas { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double debit { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double temperature { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double pH { get; set; }
    }
    public class Regul
    {
        [JsonProperty(Required = Required.Default)]
        public double sortiePID { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double consigne { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double Kp { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double Ki { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double Kd { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double sortiePID_pc { get; set; }
        [JsonProperty(Required = Required.Default)]
        public bool autorisationForcage { get; set; }
        [JsonProperty(Required = Required.Default)]
        public int consigneForcage { get; set; }
        [JsonProperty(Required = Required.Default)]
        public double offset { get; set; }

    }
    public class Condition
    {
        public int command { get; set; }
        [JsonProperty("temperature", Required = Required.Default)]
        public double temperature { get; set; }
        [JsonProperty("pH", Required = Required.Default)]
        public double pH { get; set; }
        public int condID { get; set; }
        [JsonProperty("data",Required = Required.Default)]
        public Mesocosme[] Meso { get; set; }
        [JsonProperty("regulTemp", Required = Required.Default)]
        public Regul regulTemp { get; set; }
        [JsonProperty("regulpH", Required = Required.Default)]
        public Regul regulpH { get; set; }
        public long time { get; set; }
        public DateTime lastUpdated { get; set; }

    }

    
    /// <summary>
    /// Logique d'interaction pour MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        //public List<Condition> conditions;
        public ObservableCollection<Condition> conditions;
        public ObservableCollection<Condition> conditionData;
        public Ambiant ambiantConditions = new Ambiant();

        //public List<Condition> conditionData;
        public MonitoringWindow monitoringWindow;
        public ExpSettingsWindow expSettingsWindow;
        public ComDebugWindow comDebugWindow;
        public CultureInfo ci;

        public ClientWebSocket ws = new ClientWebSocket();
        bool autoReco;
        public int step;


        public string[] Labels = new[] {"0"};
        public MainWindow()
        {
            InitializeComponent();
            var cts = new CancellationTokenSource();

            

            InitializeAsync();
            autoReco = false;
            conditions = new ObservableCollection<Condition>();
            conditionData = new ObservableCollection<Condition>();
            for(int i = 0; i < 4; i++)
            {
                Condition c = new Condition();
                c.condID = i;
                c.regulpH = new Regul();
                c.regulTemp = new Regul();
                c.Meso = new Mesocosme[3];
                for (int j = 0; j < 3; j++) c.Meso[j] = new Mesocosme();
                c.temperature = i+29.99;
                conditions.Add(c);
            }

            

            expSettingsWindow = new ExpSettingsWindow();
            comDebugWindow = new ComDebugWindow();
            monitoringWindow = new MonitoringWindow();

            comDebugWindow.lv_data.ItemsSource = conditionData;
            ci = new CultureInfo("en-US");
            ci.NumberFormat.NumberDecimalDigits = 2;
            ci.NumberFormat.NumberDecimalSeparator = ".";
            ci.NumberFormat.NumberGroupSeparator = " ";
            Thread.CurrentThread.CurrentCulture = ci;
            Thread.CurrentThread.CurrentUICulture = ci;
            CultureInfo.DefaultThreadCurrentCulture = ci;
            CultureInfo.DefaultThreadCurrentUICulture = ci;



        }





        private void saveCondtionsData(Condition cond)
        {
            string objjsonData = JsonConvert.SerializeObject(cond);
            System.IO.File.WriteAllText(@"D:\path.txt", objjsonData);
        }

        private void loadCondtionsData()
        {
            string json = System.IO.File.ReadAllText(@"D:\path.txt");
            conditions = JsonConvert.DeserializeObject <ObservableCollection<Condition>>(json);
        }

        
        private static async Task Connect(ClientWebSocket ws, TextBox tb)
        {
            string address = Properties.Settings.Default["MasterIPAddress"].ToString();
            Uri serverUri = new Uri("ws://"+address+":81");
                await ws.ConnectAsync(serverUri, CancellationToken.None);
            if (ws.State == WebSocketState.Open)
            {
                ArraySegment<byte> bytesReceived = new ArraySegment<byte>(new byte[1024]);
                WebSocketReceiveResult result = await ws.ReceiveAsync(
                    bytesReceived, CancellationToken.None);
                tb.Text = Encoding.UTF8.GetString(bytesReceived.Array, 0, result.Count);
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
            label_time.Content = DateTime.Now;
            if (expSettingsWindow.ShowActivated)
            {
                int selctedcondID = expSettingsWindow.comboBox_Condition.SelectedIndex;
                if (selctedcondID < 0 || selctedcondID > 3) selctedcondID = 0;
                expSettingsWindow.tb_pH_measure.Text = conditions[selctedcondID].pH.ToString(ci);
                expSettingsWindow.tb_pH_PIDoutput.Text = conditions[selctedcondID].regulpH.sortiePID_pc.ToString(ci);
                expSettingsWindow.tb_Temp_measure.Text = conditions[selctedcondID].temperature.ToString(ci);
                expSettingsWindow.tb_Temp_PIDoutput.Text = conditions[selctedcondID].regulTemp.sortiePID_pc.ToString(ci);
            }


            //expSettingsWindow.tb_pH_setPoint.Text = conditions[0].regulpH.consigne.ToString();
            if (command == 2)//PARAMS
            {
                
            }
            else if (command == 3)//DATA
            {


                if (conditions[0].Meso[0].alarmeNiveauHaut)
                {
                    label_C0M1_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[0].Meso[0].alarmeNiveauBas) label_C0M1_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else
                    {
                        if (!conditions[0].Meso[0].alarmeNiveauBas) label_C0M1_Alarm.Content = "Alarm: Low level";
                    }
                    label_C0M1_Alarm.Content = conditions[0].Meso[0].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }

                if (conditions[0].Meso[1].alarmeNiveauHaut)
                {
                    label_C0M2_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[0].Meso[1].alarmeNiveauBas) label_C0M2_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else
                    {
                        if (!conditions[0].Meso[1].alarmeNiveauBas) label_C0M2_Alarm.Content = "Alarm: Low level";
                    }
                    label_C0M2_Alarm.Content = conditions[0].Meso[1].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }

                if (conditions[0].Meso[2].alarmeNiveauHaut)
                {
                    label_C0M3_Alarm.Content = "Alarm: Overflow";
                }
                else
                {
                    if (ambiantConditions.tide)//vanne exondation ouverte
                    {
                        if (conditions[0].Meso[2].alarmeNiveauBas) label_C0M3_Alarm.Content = "Alarm: Exondation not effective";
                    }
                    else
                    {
                        if (!conditions[0].Meso[2].alarmeNiveauBas) label_C0M3_Alarm.Content = "Alarm: Low level";
                    }
                    label_C0M3_Alarm.Content = conditions[0].Meso[2].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }
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
                    else
                    {
                        if (!conditions[1].Meso[0].alarmeNiveauBas) label_C1M1_Alarm.Content = "Alarm: Low level";
                    }
                    label_C1M1_Alarm.Content = conditions[1].Meso[0].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
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
                    else
                    {
                        if (!conditions[1].Meso[1].alarmeNiveauBas) label_C1M2_Alarm.Content = "Alarm: Low level";
                    }
                    label_C1M2_Alarm.Content = conditions[1].Meso[1].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
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
                    else
                    {
                        if (!conditions[1].Meso[2].alarmeNiveauBas) label_C1M3_Alarm.Content = "Alarm: Low level";
                    }
                    label_C1M3_Alarm.Content = conditions[1].Meso[2].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
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
                    else
                    {
                        if (!conditions[2].Meso[0].alarmeNiveauBas) label_C2M1_Alarm.Content = "Alarm: Low level";
                    }
                    label_C2M1_Alarm.Content = conditions[2].Meso[0].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
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
                    else
                    {
                        if (!conditions[2].Meso[1].alarmeNiveauBas) label_C2M2_Alarm.Content = "Alarm: Low level";
                    }
                    label_C2M2_Alarm.Content = conditions[2].Meso[1].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
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
                    else
                    {
                        if (!conditions[2].Meso[2].alarmeNiveauBas) label_C2M3_Alarm.Content = "Alarm: Low level";
                    }
                    label_C2M3_Alarm.Content = conditions[2].Meso[2].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
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
                    else
                    {
                        if (!conditions[3].Meso[0].alarmeNiveauBas) label_C3M1_Alarm.Content = "Alarm: Low level";
                    }
                    label_C3M1_Alarm.Content = conditions[3].Meso[0].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
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
                    else
                    {
                        if (!conditions[3].Meso[1].alarmeNiveauBas) label_C3M2_Alarm.Content = "Alarm: Low level";
                    }
                    label_C3M2_Alarm.Content = conditions[3].Meso[1].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
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
                    else
                    {
                        if (!conditions[3].Meso[2].alarmeNiveauBas) label_C3M3_Alarm.Content = "Alarm: Low level";
                    }
                    label_C3M3_Alarm.Content = conditions[3].Meso[2].alarmeNiveauTresBas ? "" : "Alarm: Very Low level";
                }

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


                label_C0_pH_CO2.Content = string.Format(ci, "pH measure: {0:0.00}", conditions[0].pH);
                // label_C0_Temp.Content = string.Format(ci, "T°C: {0:0.00}°C", conditions[0].temperature);
                label_C1_pH.Content = string.Format(ci, "pH: {0:0.00}", conditions[1].pH);
                label_C1_Temp.Content = string.Format(ci, "T°C: {0:0.00}°C", conditions[1].temperature);
                label_C2_pH.Content = string.Format(ci, "pH: {0:0.00}", conditions[2].pH);
                label_C2_Temp.Content = string.Format(ci, "T°C: {0:0.00}°C", conditions[2].temperature);
                label_C3_pH.Content = string.Format(ci, "pH: {0:0.00}", conditions[3].pH);
                label_C3_Temp.Content = string.Format(ci, "T°C: {0:0.00}°C", conditions[3].temperature);

                label_C0M1_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[0].Meso[0].debit);
                label_C0M2_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[0].Meso[1].debit);
                label_C0M3_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[0].Meso[2].debit);
                label_C0M1_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[0].Meso[0].pH);
                label_C0M2_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[0].Meso[1].pH);
                label_C0M3_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[0].Meso[2].pH);
                label_C0M1_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[0].Meso[0].temperature);
                label_C0M2_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[0].Meso[1].temperature);
                label_C0M3_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[0].Meso[2].temperature);

                label_C1M1_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[1].Meso[0].debit);
                label_C1M2_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[1].Meso[1].debit);
                label_C1M3_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[1].Meso[2].debit);
                label_C1M1_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[1].Meso[0].pH);
                label_C1M2_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[1].Meso[1].pH);
                label_C1M3_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[1].Meso[2].pH);
                label_C1M1_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[1].Meso[0].temperature);
                label_C1M2_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[1].Meso[1].temperature);
                label_C1M3_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[1].Meso[2].temperature);

                label_C2M1_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[2].Meso[0].debit);
                label_C2M2_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[2].Meso[1].debit);
                label_C2M3_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[2].Meso[2].debit);
                label_C2M1_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[2].Meso[0].pH);
                label_C2M2_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[2].Meso[1].pH);
                label_C2M3_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[2].Meso[2].pH);
                label_C2M1_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[2].Meso[0].temperature);
                label_C2M2_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[2].Meso[1].temperature);
                label_C2M3_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[2].Meso[2].temperature);

                label_C3M1_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[3].Meso[0].debit);
                label_C3M2_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[3].Meso[1].debit);
                label_C3M3_Flowrate.Content = string.Format(ci, "Flowrate: {0:0.00}l/s", conditions[3].Meso[2].debit);
                label_C3M1_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[3].Meso[0].pH);
                label_C3M2_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[3].Meso[1].pH);
                label_C3M3_pH.Content = string.Format(ci, "pH: \t{0:0.00}", conditions[3].Meso[2].pH);
                label_C3M1_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[3].Meso[0].temperature);
                label_C3M2_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[3].Meso[1].temperature);
                label_C3M3_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}°C", conditions[3].Meso[2].temperature);
            }
            else if (command == 6)//MASTER DATA
            {
                label_C0_02.Content = string.Format(ci, "O2: \t{0:0.00}", ambiantConditions.oxy);
                label_C0_Cond.Content = string.Format(ci, "Cond: \t{0:0.00}", ambiantConditions.cond);
                label_C0_Turb.Content = string.Format(ci, "Turb: \t{0:0.00}", ambiantConditions.turb);
                label_C0_Fluo.Content = string.Format(ci, "Fluo: \t{0:0.00}", ambiantConditions.fluo);
                label_C0_Temp.Content = string.Format(ci, "T°C: \t{0:0.00}", ambiantConditions.temperature);
                label_C0_pH.Content = string.Format(ci, "pH: \t{0:0.00}", ambiantConditions.pH);
                if (ambiantConditions.tide) label_exondation_state.Content = string.Format(ci, "Exondation Valve: OPEN (low tide)");
                else string.Format(ci, "Exondation Valve: CLOSED (high tide)");
                if (ambiantConditions.sun) label_ledstate.Content = string.Format(ci, "LED state: ON (Day)");
                else string.Format(ci, "LED state: OFF (Night)");
            }
        }



        private void checkConnection()
        {
            switch (ws.State)
            {
                case WebSocketState.Open:
                    Connect_btn.Header = "Disconnect";
                    Connect_btn.IsEnabled = true;
                    break;
                case WebSocketState.Closed:
                case WebSocketState.Aborted:
                case WebSocketState.None:
                    Connect_btn.Header = "Connect";
                    Connect_btn.IsEnabled = true;
                    break;
                case WebSocketState.Connecting:
                    Connect_btn.Header = "Connecting";
                    Connect_btn.IsEnabled = false;
                    break;
            }
            
            statusLabel.Text = "Connection Status: " + ws.State.ToString();

            string msg = "";
            if (conditions[0].regulpH.consigne == 0) msg = "{command:0,condID:0, senderID:4}";
            else
            {
                switch (step)
                {
                    case 0:
                        msg = "{command:1,condID:0, senderID:4}";
                        break;
                    case 1:
                        msg = "{command:1,condID:1, senderID:4}";
                        break;
                    case 2:
                        msg = "{command:1,condID:2, senderID:4}";
                        break;
                    case 3:
                        msg = "{command:1,condID:3, senderID:4}";
                        break;
                    case 4:
                        var Timestamp = new DateTimeOffset(DateTime.UtcNow).ToUnixTimeSeconds();
                        msg = "{command:5,condID:0, senderID:4, time:" + Timestamp + "}";
                        break;
                }
                if (step < 4) step++; else step = 0;
            }
            

            comDebugWindow.tb1.Text = msg;

            if (ws.State == WebSocketState.Open)
            {
                Task<string> t2 = Send(ws, msg, comDebugWindow.tb2);
                t2.Wait(50);
            }
            else
            {
                Connect();
            }
            
        }

        private void Connect()
        {
            
            if (ws.State == WebSocketState.None)
            {
                Task t = Connect(ws, comDebugWindow.tb2);
                t.Wait(50);
                Connect_btn.Header = "Connecting";
                Connect_btn.IsEnabled = false;
                statusLabel.Text = "Connection Status: " + ws.State.ToString();
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
            while (!token.IsCancellationRequested)
            {
                // Call our onTick function.
                onTick?.Invoke();

                // Wait to repeat again.
                if (interval > TimeSpan.Zero)
                    await Task.Delay(interval, token);
            }
        }

        private async Task InitializeAsync()
        {
            int t;
            Int32.TryParse(Properties.Settings.Default["dataQueryInterval"].ToString(), out t);
            var dueTime = TimeSpan.FromSeconds(t);
            var interval = TimeSpan.FromSeconds(t);

            // TODO: Add a CancellationTokenSource and supply the token here instead of None.
            await RunPeriodicAsync(checkConnection, dueTime, interval, CancellationToken.None);
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
            expSettingsWindow.Show();
        }

        private void Calibrate_btn_Click(object sender, RoutedEventArgs e)
        {

        }

        private void CleanUp_Click(object sender, RoutedEventArgs e)
        {

        }

        private void ManualOverride_Click(object sender, RoutedEventArgs e)
        {

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
            monitoringWindow.Close();
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
            monitoringWindow.Show();
        }
    }
}

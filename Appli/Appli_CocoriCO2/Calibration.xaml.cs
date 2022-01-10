﻿using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Net.WebSockets;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace Appli_CocoriCO2
{
    /// <summary>
    /// Logique d'interaction pour Calibration.xaml
    /// </summary>
    public partial class Calibration : Window
    {
        MainWindow MW = ((MainWindow)Application.Current.MainWindow);
        CancellationTokenSource cts = new CancellationTokenSource();



        public Calibration()
        {
            InitializeComponent();
            comboBox_Condition.SelectedIndex = 0;
            comboBox_Sensor.SelectedIndex = 0;
            InitializeAsync();
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
            Int32.TryParse(Properties.Settings.Default["dataLogInterval"].ToString(), out t);
            var dueTime = TimeSpan.FromMinutes(0);
            var interval = TimeSpan.FromSeconds(1);

            // TODO: Add a CancellationTokenSource and supply the token here instead of None.
            await RunPeriodicAsync(RefreshMeasure, dueTime, interval, cts.Token);
        }
        private void RefreshUI()
        {
            
                    double t;
            int condID = comboBox_Condition.SelectedIndex;
            int sensorID; // HAMILTON: indexes O to 2 are mesocosms, index 3 is acidification tank, index 4 is input measure tank, 5 = oxy, 6 = NTU, 7 = Cond
            if (condID > 0)
            {
                comboBox_Sensor.Visibility = Visibility.Visible;
                comboBox_Sensor_C0.Visibility = Visibility.Hidden;
                sensorID = comboBox_Sensor.SelectedIndex;
            }
            else
            {
                comboBox_Sensor.Visibility = Visibility.Hidden;
                comboBox_Sensor_C0.Visibility = Visibility.Visible;
                sensorID = comboBox_Sensor_C0.SelectedIndex;
            }

            switch (sensorID)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                    label_measure2.Content = "";
                    label_measure3.Content = "";
                    label_explain_offset.Content = "Enter the actual pH value";
                    label_explain_slope.Content = "";
                    lbl_std1.Content = "Corrected pH";
                    lbl_std2.Visibility = Visibility.Hidden;
                    tb_Slope.Visibility = Visibility.Hidden;
                    btn_SendSlope.Visibility = Visibility.Hidden;
                    tb_Offset.IsEnabled = true;
                    tb_Slope.IsEnabled = true;
                    break;
                case 5:
                    label_measure.Content = string.Format("{0:0.00}", MW.ambiantConditions.oxy);
                    label_measure2.Content = "";
                    label_measure3.Content = "";
                    label_explain_offset.Content = "Has to be 0%";
                    label_explain_slope.Content = "Has to be 100%";
                    tb_Offset.Text = "0";
                    tb_Slope.Text = "100";
                    tb_Offset.IsEnabled = false;
                    tb_Slope.IsEnabled = false;
                    lbl_std1.Content = "Calibration standard 1";
                    lbl_std2.Content = "Calibration standard 2";
                    lbl_std2.Visibility = Visibility.Visible;
                    tb_Slope.Visibility = Visibility.Visible;
                    btn_SendSlope.Visibility = Visibility.Visible;
                    break;
                case 6:
                    label_measure.Content = string.Format("{0:0.00} ", MW.ambiantConditions.turb);
                    label_measure2.Content = "";
                    label_measure3.Content = "";
                    label_explain_offset.Content = "Ideally a value close to 0";
                    label_explain_slope.Content = "Ideally a value close to 25";
                    tb_Offset.IsEnabled = true;
                    tb_Slope.IsEnabled = true;
                    lbl_std1.Content = "Calibration standard 1";
                    lbl_std2.Content = "Calibration standard 2";
                    lbl_std2.Visibility = Visibility.Visible;
                    tb_Slope.Visibility = Visibility.Visible;
                    btn_SendSlope.Visibility = Visibility.Visible;
                    break;
                case 7:
                    label_measure.Content = string.Format("{0:0.00} uS/cm", MW.ambiantConditions.cond);
                    label_measure2.Content = string.Format("{0:0.00}", MW.ambiantConditions.salinite);
                    label_measure3.Content = "";
                    label_explain_offset.Content = "Ideally a value close to 0 µS/cm";
                    label_explain_slope.Content = "Has to be above 20 000 µS/cm";
                    tb_Offset.IsEnabled = true;
                    tb_Slope.IsEnabled = true;
                    lbl_std1.Content = "Calibration standard 1";
                    lbl_std2.Content = "Calibration standard 2";
                    lbl_std2.Visibility = Visibility.Visible;
                    tb_Slope.Visibility = Visibility.Visible;
                    btn_SendSlope.Visibility = Visibility.Visible;
                    break;
                case 8://FLuo
                    label_measure.Content = string.Format("{0:0.000} ", MW.ambiantConditions.fluo);
                    label_measure2.Content = "";
                    label_measure3.Content = "";
                    tb_Offset.IsEnabled = true;
                    tb_Slope.IsEnabled = true;
                    Double.TryParse(Properties.Settings.Default["FluoOffset"].ToString(), out t);

                    tb_Offset.Text = string.Format("{0:0.000} ", t);
                    Double.TryParse(Properties.Settings.Default["FluoSlope"].ToString(), out t);
                    tb_Slope.Text = string.Format("{0:0.000} ", t);
                    lbl_std1.Content = "Offset";
                    lbl_std2.Content = "Slope";
                    lbl_std2.Visibility = Visibility.Visible;
                    tb_Slope.Visibility = Visibility.Visible;
                    btn_SendSlope.Visibility = Visibility.Visible;
                    break;
            }
        }
            private void RefreshMeasure()
        {

            int condID = comboBox_Condition.SelectedIndex;
            int sensorID;// HAMILTON: indexes O to 2 are mesocosms, index 3 is acidification tank, index 4 is input measure tank, 5 = oxy, 6 = NTU, 7 = Cond
            if (condID > 0) {
                sensorID = comboBox_Sensor.SelectedIndex;
            } else {
                sensorID = comboBox_Sensor_C0.SelectedIndex;
            }
            
            switch (sensorID)
            {
                case 0:
                    label_measure.Content = string.Format("{0:0.00}", MW.conditions[condID].Meso[sensorID].pH);
                    break;
                case 1:
                    label_measure.Content = string.Format("{0:0.00}", MW.conditions[condID].Meso[sensorID].pH);
                    break;
                case 2:
                    label_measure.Content = string.Format("{0:0.00}", MW.conditions[condID].Meso[sensorID].pH);
                    break;
                case 3:
                    label_measure.Content = string.Format("{0:0.00}", MW.conditions[condID].pH);
                    break;
                case 4:
                    label_measure.Content = string.Format("{0:0.00}", MW.ambiantConditions.pH);
                    break;
                case 5:
                    label_measure.Content = string.Format("{0:0.00}", MW.ambiantConditions.oxy);
                   
                    break;
                case 6:
                    label_measure.Content = string.Format("{0:0.00} ", MW.ambiantConditions.turb);
                    
                    break;
                case 7:
                    label_measure.Content = string.Format("{0:0.00} ", MW.ambiantConditions.cond);
                    label_measure2.Content = string.Format("{0:0.00}", MW.ambiantConditions.salinite);
                    
                    break;
                case 8:
                    label_measure.Content = string.Format("{0:0.000} ", MW.ambiantConditions.fluo);

                    break;


            }
        }


        private void btn_SendOffset_Click(object sender, RoutedEventArgs e)
        {
            int condID = comboBox_Condition.SelectedIndex;
            int sensorID;// HAMILTON: indexes O to 2 are mesocosms, index 3 is acidification tank, index 4 is input measure tank, 5 = oxy, 6 = NTU, 7 = Cond
            if (condID > 0)
            {
                sensorID = comboBox_Sensor.SelectedIndex;
            }
            else
            {
                sensorID = comboBox_Sensor_C0.SelectedIndex;
            }

            float value;

            float.TryParse(tb_Offset.Text, out value);
            if (sensorID == 8)
            {
                Properties.Settings.Default["FluoOffset"] = value.ToString();
                Properties.Settings.Default.Save();

            }
            else sendReq(condID, sensorID, 0, value);
        }

        private void btn_SendSlope_Click(object sender, RoutedEventArgs e)
        {
            int condID = comboBox_Condition.SelectedIndex;
            int sensorID = comboBox_Sensor_C0.SelectedIndex;
            float value;

            float.TryParse(tb_Slope.Text, out value);
            if (sensorID == 8)
            {
                Properties.Settings.Default["FluoSlope"] = value.ToString();
                Properties.Settings.Default.Save();

            }
            else
                sendReq(condID, sensorID, 1, value);
        }

        private void sendReq(int condID, int sensorID, int calibParam, float value)
        {
            //{ command: 4, condID: 1,senderID: 4, MesoID: 1,sensorID: 2, calibParam: 1, value: 123,45}
            string msg = "{command:4,condID:" + condID + ",senderID:4,sensorID:" + sensorID + ",calibParam:" + calibParam + ",value:" + value + "}";

            ((MainWindow)Application.Current.MainWindow).comDebugWindow.tb1.Text = msg;


            if (((MainWindow)Application.Current.MainWindow).ws.State == WebSocketState.Open)
            {
                Task<string> t2 = Send(((MainWindow)Application.Current.MainWindow).ws, msg, ((MainWindow)Application.Current.MainWindow).comDebugWindow.tb2);
                t2.Wait(50);
            }
        }

        private void comboBox_Sensor_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            RefreshUI();
        }

        private void comboBox_Condition_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            comboBox_Sensor_SelectionChanged(sender, e);
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
            }
            if (ws.State == WebSocketState.Open)
            {
                ArraySegment<byte> bytesReceived = new ArraySegment<byte>(new byte[600]);
                WebSocketReceiveResult result = ws.ReceiveAsync(
                    bytesReceived, timeOut).Result;
                string data = Encoding.UTF8.GetString(bytesReceived.Array, 0, result.Count);
                tb.Text = data;
                return data;
            }
            return null;

        }

        private void btn_FactoryReset_Click(object sender, RoutedEventArgs e)
        {
            int condID = comboBox_Condition.SelectedIndex;
            int sensorID = comboBox_Sensor.SelectedIndex;
            float value;

            float.TryParse(tb_Offset.Text, out value);
            sendReq(condID, sensorID, 99, value);
        }

        private void Window_Closing_1(object sender, System.ComponentModel.CancelEventArgs e)
        {
            this.Hide();
            e.Cancel = true;
        }
    }
}

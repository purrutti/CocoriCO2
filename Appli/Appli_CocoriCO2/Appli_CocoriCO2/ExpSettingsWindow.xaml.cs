using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Net.WebSockets;
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
    /// Logique d'interaction pour ExpSettingsWindow.xaml
    /// </summary>
    public partial class ExpSettingsWindow : Window
    {
        MainWindow MW = ((MainWindow)Application.Current.MainWindow);
        public CultureInfo ci;
        public ExpSettingsWindow()
        {
            InitializeComponent();
            ci = new CultureInfo("en-US");
            ci.NumberFormat.NumberDecimalDigits = 2;
            ci.NumberFormat.NumberDecimalSeparator = ".";
            ci.NumberFormat.NumberGroupSeparator = " ";
            Thread.CurrentThread.CurrentCulture = ci;
            Thread.CurrentThread.CurrentUICulture = ci;
            CultureInfo.DefaultThreadCurrentCulture = ci;
            CultureInfo.DefaultThreadCurrentUICulture = ci;

            comboBox_Condition.SelectedIndex = 0;
        }

        private void btn_SaveToFile_Click(object sender, RoutedEventArgs e)
        {

        }

        private void btn_LoadFromPLC_Click(object sender, RoutedEventArgs e)
        {
            load(comboBox_Condition.SelectedIndex);
        }

        public async void load(int index)
        {
            string msg;
            foreach (var wsd in ((MainWindow)Application.Current.MainWindow)._sockets)
            {
                var ws = wsd;
                if (ws.IsAvailable) { 
                if (index == 4)
                    {

                        msg = "{\"cmd\":7,\"cID\":0, \"sID\":4}";
                    }
                    else if (index == 5)
                    {

                        msg = "{\"cmd\":10,\"cID\":0, \"sID\":4}";
                    }
                    /*else if (index == 6)
                    {

                        msg = "{\"cmd\":0}";
                    }*/
                    else
                    {
                        //{command:0,condID:0,senderID:4}
                        msg = "{\"cmd\":0,\"cID\":" + index + ", \"sID\":4}";
                    }
                    try
                    {

                        await ws.Send(msg);
                    }
                    catch(Exception e) { }

                }
            }

        }

        private async void btn_SaveToPLC_Click(object sender, RoutedEventArgs e)
        {
            int temp;
            double dTemp;
            string msg;
            foreach (var wsd in ((MainWindow)Application.Current.MainWindow)._sockets)
            {
                var ws = wsd;
                if (ws.IsAvailable)
                {
                    if (comboBox_Condition.SelectedIndex == 5)
                    {

                        MW.pacParams.rTempEC.autorisationForcage = (bool)checkBox_pH_Override.IsChecked;
                        if (Int32.TryParse(tb_pH_consigneForcage.Text, out temp)) MW.pacParams.rTempEC.consigneForcage = temp;
                        if (Double.TryParse(tb_dpH_setPoint.Text, out dTemp)) MW.pacParams.rTempEC.offset = dTemp;
                        MW.pacParams.rTempEC.consigne = MW.ambiantConditions.temperature + MW.pacParams.rTempEC.offset;
                        if (Double.TryParse(tb_pH_Kp.Text, out dTemp)) MW.pacParams.rTempEC.Kp = dTemp;
                        if (Double.TryParse(tb_pH_Ki.Text, out dTemp)) MW.pacParams.rTempEC.Ki = dTemp;
                        if (Double.TryParse(tb_pH_Kd.Text, out dTemp)) MW.pacParams.rTempEC.Kd = dTemp;
                        var response = new
                        {
                            cmd = 9,
                            cID = 0,
                            sID = 4,//Server
                            MW.pacParams.rTempEC
                        };

                        String s = JsonConvert.SerializeObject(response);


                        await ws.Send(s);





                        /*
                                            msg = "{cmd:9,cID:0,sID:4,";
                                            msg += "\"rTempEC\":{";
                                            msg += "\"offset\":" + MW.pacParams.rTempEC.offset.ToString() + ",";
                                            msg += "\"cons\":" + MW.pacParams.rTempEC.consigne.ToString() + ",";
                                            msg += "\"Kp\":" + MW.pacParams.rTempEC.Kp.ToString() + ",";
                                            msg += "\"Ki\":" + MW.pacParams.rTempEC.Ki.ToString() + ",";
                                            msg += "\"Kd\":" + MW.pacParams.rTempEC.Kd.ToString() + ",";
                                            msg += "\"consForcage\":" + MW.pacParams.rTempEC.consigneForcage + ",";
                                            msg += "\"aForcage\":\"" + MW.pacParams.rTempEC.autorisationForcage + "\"}";
                                            msg += "}";*/
                    }
                    else if (comboBox_Condition.SelectedIndex == 4)
                    {
                        MW.masterParams.regulPressionEA.autorisationForcage = (bool)checkBox_pH_Override.IsChecked;
                        if (Int32.TryParse(tb_pH_consigneForcage.Text, out temp)) MW.masterParams.regulPressionEA.consigneForcage = temp;
                        if (Double.TryParse(tb_pH_setPoint.Text, out dTemp)) MW.masterParams.regulPressionEA.consigne = dTemp;
                        if (Double.TryParse(tb_pH_Kp.Text, out dTemp)) MW.masterParams.regulPressionEA.Kp = dTemp;
                        if (Double.TryParse(tb_pH_Ki.Text, out dTemp)) MW.masterParams.regulPressionEA.Ki = dTemp;
                        if (Double.TryParse(tb_pH_Kd.Text, out dTemp)) MW.masterParams.regulPressionEA.Kd = dTemp;

                        MW.masterParams.regulPressionEC.autorisationForcage = (bool)checkBox_Temp_Override.IsChecked;
                        if (Int32.TryParse(tb_Temp_consigneForcage.Text, out temp)) MW.masterParams.regulPressionEC.consigneForcage = temp;
                        if (Double.TryParse(tb_Temp_setPoint.Text, out dTemp)) MW.masterParams.regulPressionEC.consigne = dTemp;
                        if (Double.TryParse(tb_Temp_Kp.Text, out dTemp)) MW.masterParams.regulPressionEC.Kp = dTemp;
                        if (Double.TryParse(tb_Temp_Ki.Text, out dTemp)) MW.masterParams.regulPressionEC.Ki = dTemp;
                        if (Double.TryParse(tb_Temp_Kd.Text, out dTemp)) MW.masterParams.regulPressionEC.Kd = dTemp;
                        if (Double.TryParse(tb_dT_setPoint.Text, out dTemp)) MW.masterParams.regulPressionEC.offset = dTemp;

                        var response = new
                        {
                            cmd = 8,
                            cID = 0,
                            sID = 4,//Server
                            rPressionEA = MW.masterParams.regulPressionEA,
                            rPressionEC = MW.masterParams.regulPressionEC,
                        };

                        String s = JsonConvert.SerializeObject(response);

                        await ws.Send(s);


                        /*msg = "{cmd:8,cID:0,sID:4,";
                        msg += "\"rPressionEA\":{";
                        msg += "\"cons\":" + MW.masterParams.regulPressionEA.consigne.ToString() + ",";
                        msg += "\"Kp\":" + MW.masterParams.regulPressionEA.Kp.ToString() + ",";
                        msg += "\"Ki\":" + MW.masterParams.regulPressionEA.Ki.ToString() + ",";
                        msg += "\"Kd\":" + MW.masterParams.regulPressionEA.Kd.ToString() + ",";
                        msg += "\"consForcage\":" + MW.masterParams.regulPressionEA.consigneForcage + ",";
                        msg += "\"aForcage\":\"" + MW.masterParams.regulPressionEA.autorisationForcage + "\"},";

                        msg += "\"rPressionEC\":{";
                        msg += "\"cons\":" + MW.masterParams.regulPressionEC.consigne.ToString() + ",";
                        msg += "\"Kp\":" + MW.masterParams.regulPressionEC.Kp.ToString() + ",";
                        msg += "\"Ki\":" + MW.masterParams.regulPressionEC.Ki.ToString() + ",";
                        msg += "\"Kd\":" + MW.masterParams.regulPressionEC.Kd.ToString() + ",";
                        msg += "\"consForcage\":" + MW.masterParams.regulPressionEC.consigneForcage + ",";
                        msg += "\"aForcage\":\"" + MW.masterParams.regulPressionEC.autorisationForcage + "\"}";
                        msg += "}";*/
                         }
                         else
                         {

                        Condition c = MW.conditions[comboBox_Condition.SelectedIndex];
                             c.condID = comboBox_Condition.SelectedIndex;
                             c.command = 2;
                             c.rpH = new Regul();
                             c.rpH.autorisationForcage = (bool)checkBox_pH_Override.IsChecked;
                             if (Int32.TryParse(tb_pH_consigneForcage.Text, out temp)) c.rpH.consigneForcage = temp;
                             if (Double.TryParse(tb_pH_Kp.Text, out dTemp)) c.rpH.Kp = dTemp;
                             if (Double.TryParse(tb_pH_Ki.Text, out dTemp)) c.rpH.Ki = dTemp;
                             if (Double.TryParse(tb_pH_Kd.Text, out dTemp)) c.rpH.Kd = dTemp;
                             if (Double.TryParse(tb_dpH_setPoint.Text, out dTemp)) c.rpH.offset = dTemp;
                                c.rpH.consigne = MW.ambiantConditions.pH + c.rpH.offset;


                        if (c.condID > 0)
                             {
                                 c.rTemp = new Regul();
                                 c.rTemp.autorisationForcage = (bool)checkBox_Temp_Override.IsChecked;
                                 if (Int32.TryParse(tb_Temp_consigneForcage.Text, out temp)) c.rTemp.consigneForcage = temp;
                                 if (Double.TryParse(tb_Temp_setPoint.Text, out dTemp)) c.rTemp.consigne = dTemp;
                                 if (Double.TryParse(tb_Temp_Kp.Text, out dTemp)) c.rTemp.Kp = dTemp;
                                 if (Double.TryParse(tb_Temp_Ki.Text, out dTemp)) c.rTemp.Ki = dTemp;
                                 if (Double.TryParse(tb_Temp_Kd.Text, out dTemp)) c.rTemp.Kd = dTemp;
                                 if (Double.TryParse(tb_dT_setPoint.Text, out dTemp)) c.rTemp.offset = dTemp;

                            c.rTemp.consigne = MW.ambiantConditions.temperature + c.rTemp.offset;
                        }

                             var response = new
                             {
                                 cmd = 2,
                                 cID = c.condID,
                                 sID = 4,//Server
                                 c.rpH,
                                 c.rTemp
                             };

                             String s = JsonConvert.SerializeObject(response);

                        await ws.Send(s);




                        /*
                         * {"command":2,"condID":0,"time":"1611595972","rTemp":{"consigne":0,"Kp":0,"Ki":0,"Kd":0},"rpH":{"consigne":0,"Kp":0,"Ki":0,"Kd":0}}
                         * */
                        /* msg = "{cmd:2,cID:" + comboBox_Condition.SelectedIndex + ",sID:4,";

                         if (c.condID > 0)
                         {
                             c.rTemp = new Regul();
                             c.rTemp.autorisationForcage = (bool)checkBox_Temp_Override.IsChecked;
                             if (Int32.TryParse(tb_Temp_consigneForcage.Text, out temp)) c.rTemp.consigneForcage = temp;
                             if (Double.TryParse(tb_Temp_setPoint.Text, out dTemp)) c.rTemp.consigne = dTemp;
                             if (Double.TryParse(tb_Temp_Kp.Text, out dTemp)) c.rTemp.Kp = dTemp;
                             if (Double.TryParse(tb_Temp_Ki.Text, out dTemp)) c.rTemp.Ki = dTemp;
                             if (Double.TryParse(tb_Temp_Kd.Text, out dTemp)) c.rTemp.Kd = dTemp;
                             if (Double.TryParse(tb_dT_setPoint.Text, out dTemp)) c.rTemp.offset = dTemp;
                             msg += "\"rTemp\":{";
                             msg += "\"offset\":" + c.rTemp.offset.ToString() + ",";
                             msg += "\"cons\":" + c.rTemp.consigne.ToString() + ",";
                             msg += "\"Kp\":" + c.rTemp.Kp.ToString() + ",";
                             msg += "\"Ki\":" + c.rTemp.Ki.ToString() + ",";
                             msg += "\"Kd\":" + c.rTemp.Kd.ToString() + ",";
                             msg += "\"consForcage\":" + c.rTemp.consigneForcage + ",";
                             msg += "\"aForcage\":\"" + c.rTemp.autorisationForcage + "\"},";

                         }

                         msg += "\"rpH\":{\"offset\":" + c.rpH.offset.ToString() + ",";
                         msg += "\"cons\":" + c.rpH.consigne.ToString() + ",";
                         msg += "\"Kp\":" + c.rpH.Kp.ToString() + ",";
                         msg += "\"Ki\":" + c.rpH.Ki.ToString() + ",";
                         msg += "\"Kd\":" + c.rpH.Kd.ToString() + ",";
                         msg += "\"consForcage\":" + c.rpH.consigneForcage + ",";
                         msg += "\"aForcage\":\"" + c.rpH.autorisationForcage + "\"}";

                         msg += "}";
                     }
                     Task<string> t2 = Send(((MainWindow)Application.Current.MainWindow).ws, msg, ((MainWindow)Application.Current.MainWindow).comDebugWindow.tb2);
                     t2.Wait(50);*/
                        }
                     }
                 }
            refreshParams();
        }

        private void btn_Cancel_Click(object sender, RoutedEventArgs e)
        {
            this.Hide();
        }

       

        private void comboBox_Condition_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            load(comboBox_Condition.SelectedIndex);
            Visibility v;
            if (comboBox_Condition.SelectedIndex == 5)
            {
                label_pH_title.Content = "Hot water temperature";
                label_pH_setpoint.Content = "Temperature setpoint";
                label_dpH.Content = "delta T°C setpoint";
                label_pH_measure.Content = "Temperature measure";
                tb_pH_setPoint.IsEnabled = false;


                v = Visibility.Hidden;
                label_Temp_title.Visibility = v;
                tb_dpH_setPoint.Visibility = Visibility.Visible;
                label_dpH.Visibility = Visibility.Visible;
                tb_dT_setPoint.Visibility = Visibility.Hidden;
                label_dT.Visibility = Visibility.Hidden;
                tb_Temp_setPoint.Visibility = v;
                tb_Temp_Kp.Visibility = v;
                tb_Temp_Ki.Visibility = v;
                label_Temp_Kp.Visibility = v;
                label_Temp_setpoint.Visibility = v;
                label_Temp_Ki.Visibility = v;
                tb_Temp_Kd.Visibility = v;
                label_Temp_Kd.Visibility = v;
                tb_Temp_measure.Visibility = v;
                label_Temp_measure.Visibility = v;
                tb_Temp_PIDoutput.Visibility = v;
                label_Temp_sortiePID.Visibility = v;
                label_pc1.Visibility = v;
                label_pc2.Visibility = v;
                tb_Temp_consigneForcage.Visibility = v;
                checkBox_Temp_Override.Visibility = v;

            }
            else if (comboBox_Condition.SelectedIndex == 4)
            {
                label_pH_title.Content = "Ambient water pressure";
                label_Temp_title.Content = "Hot water pressure";
                label_pH_setpoint.Content = "pressure setpoint";
                label_Temp_setpoint.Content = "pressure setpoint";
                label_pH_measure.Content = "pressure measure";
                label_Temp_measure.Content = "pressure measure";
                tb_pH_setPoint.IsEnabled = true;
                tb_Temp_setPoint.IsEnabled = true;


                v = Visibility.Visible;
                label_Temp_title.Visibility = v;
                tb_dpH_setPoint.Visibility = Visibility.Hidden;
                label_dpH.Visibility = Visibility.Hidden;
                tb_dT_setPoint.Visibility = Visibility.Hidden;
                label_dT.Visibility = Visibility.Hidden;
                tb_Temp_setPoint.Visibility = v;
                tb_Temp_Kp.Visibility = v;
                tb_Temp_Ki.Visibility = v;
                label_Temp_Kp.Visibility = v;
                label_Temp_setpoint.Visibility = v;
                label_Temp_Ki.Visibility = v;
                tb_Temp_Kd.Visibility = v;
                label_Temp_Kd.Visibility = v;
                tb_Temp_measure.Visibility = v;
                label_Temp_measure.Visibility = v;
                tb_Temp_PIDoutput.Visibility = v;
                label_Temp_sortiePID.Visibility = v;
                label_pc1.Visibility = v;
                label_pc2.Visibility = v;
                tb_Temp_consigneForcage.Visibility = v;
                checkBox_Temp_Override.Visibility = v;

            }
            else

            {
                label_dpH.Content = "delta pHsetpoint";
                label_pH_setpoint.Content = "pH setpoint";
                label_Temp_setpoint.Content = "Temperature setpoint";
                label_pH_measure.Content = "pH measure";
                label_Temp_measure.Content = "Temperature measure";
                label_Temp_title.Content = "Temperature regulation";
                if (comboBox_Condition.SelectedIndex == 0)
                {
                    v = Visibility.Hidden;
                    tb_pH_setPoint.IsEnabled = true;
                    label_pH_title.Content = "CO2 valve regulation";
                }
                else
                {
                    v = Visibility.Visible;
                    tb_pH_setPoint.IsEnabled = false;
                    tb_Temp_setPoint.IsEnabled = false;
                    label_pH_title.Content = "pH regulation";
                }
                label_Temp_title.Visibility = v;
                tb_dpH_setPoint.Visibility = v;
                label_dpH.Visibility = v;
                tb_Temp_setPoint.Visibility = v;
                tb_Temp_Kp.Visibility = v;
                tb_Temp_Ki.Visibility = v;
                label_Temp_Kp.Visibility = v;
                label_Temp_setpoint.Visibility = v;
                label_Temp_Ki.Visibility = v;
                tb_Temp_Kd.Visibility = v;
                label_Temp_Kd.Visibility = v;
                tb_Temp_measure.Visibility = v;
                label_Temp_measure.Visibility = v;
                tb_Temp_PIDoutput.Visibility = v;
                label_Temp_sortiePID.Visibility = v;
                label_pc1.Visibility = v;
                label_pc2.Visibility = v;
                tb_Temp_consigneForcage.Visibility = v;
                checkBox_Temp_Override.Visibility = v;
                tb_dT_setPoint.Visibility = v;
                label_dT.Visibility = v;
            }
            refreshParams();
        }

        public void refreshParams()
        {
            try
            {

                Dispatcher.Invoke(() =>
                {
                    if (comboBox_Condition.SelectedIndex < 4)
                    {
                        int condID = comboBox_Condition.SelectedIndex;
                        tb_dpH_setPoint.Text = MW.conditions[condID].rpH.offset.ToString(ci);
                        tb_pH_setPoint.Text = MW.conditions[condID].rpH.consigne.ToString(ci);
                        tb_pH_consigneForcage.Text = MW.conditions[condID].rpH.consigneForcage.ToString(ci);
                        tb_pH_Kp.Text = MW.conditions[condID].rpH.Kp.ToString(ci);
                        tb_pH_Ki.Text = MW.conditions[condID].rpH.Ki.ToString(ci);
                        tb_pH_Kd.Text = MW.conditions[condID].rpH.Kd.ToString(ci);
                        checkBox_pH_Override.IsChecked = MW.conditions[condID].rpH.autorisationForcage;
                        if (condID > 0)
                        {
                            tb_dT_setPoint.Text = MW.conditions[condID].rTemp.offset.ToString(ci);
                            tb_Temp_setPoint.Text = MW.conditions[condID].rTemp.consigne.ToString(ci);
                            tb_Temp_consigneForcage.Text = MW.conditions[condID].rTemp.consigneForcage.ToString(ci);
                            tb_Temp_Kp.Text = MW.conditions[condID].rTemp.Kp.ToString(ci);
                            tb_Temp_Ki.Text = MW.conditions[condID].rTemp.Ki.ToString(ci);
                            tb_Temp_Kd.Text = MW.conditions[condID].rTemp.Kd.ToString(ci);
                            checkBox_Temp_Override.IsChecked = MW.conditions[condID].rTemp.autorisationForcage;
                        }
                    }
                    else
                    {
                        if (comboBox_Condition.SelectedIndex == 4)
                        {
                            tb_dpH_setPoint.Text = MW.masterParams.regulPressionEA.offset.ToString(ci);
                            tb_pH_setPoint.Text = MW.masterParams.regulPressionEA.consigne.ToString(ci);
                            tb_pH_consigneForcage.Text = MW.masterParams.regulPressionEA.consigneForcage.ToString(ci);
                            tb_pH_Kp.Text = MW.masterParams.regulPressionEA.Kp.ToString(ci);
                            tb_pH_Ki.Text = MW.masterParams.regulPressionEA.Ki.ToString(ci);
                            tb_pH_Kd.Text = MW.masterParams.regulPressionEA.Kd.ToString(ci);
                            checkBox_pH_Override.IsChecked = MW.masterParams.regulPressionEA.autorisationForcage;

                            tb_dT_setPoint.Text = MW.masterParams.regulPressionEC.offset.ToString(ci);
                            tb_Temp_setPoint.Text = MW.masterParams.regulPressionEC.consigne.ToString(ci);
                            tb_Temp_consigneForcage.Text = MW.masterParams.regulPressionEC.consigneForcage.ToString(ci);
                            tb_Temp_Kp.Text = MW.masterParams.regulPressionEC.Kp.ToString(ci);
                            tb_Temp_Ki.Text = MW.masterParams.regulPressionEC.Ki.ToString(ci);
                            tb_Temp_Kd.Text = MW.masterParams.regulPressionEC.Kd.ToString(ci);
                            checkBox_Temp_Override.IsChecked = MW.masterParams.regulPressionEC.autorisationForcage;
                        }
                        else//=5
                        {
                            tb_dpH_setPoint.Text = MW.pacParams.rTempEC.offset.ToString(ci);
                            tb_pH_setPoint.Text = MW.pacParams.rTempEC.consigne.ToString(ci);
                            tb_pH_consigneForcage.Text = MW.pacParams.rTempEC.consigneForcage.ToString(ci);
                            tb_pH_Kp.Text = MW.pacParams.rTempEC.Kp.ToString(ci);
                            tb_pH_Ki.Text = MW.pacParams.rTempEC.Ki.ToString(ci);
                            tb_pH_Kd.Text = MW.pacParams.rTempEC.Kd.ToString(ci);
                            checkBox_pH_Override.IsChecked = MW.pacParams.rTempEC.autorisationForcage;
                        }


                    }
                });
            }
            catch (Exception e)
            {

            }


        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            this.Hide();
            e.Cancel = true;
        }
    }
}
using LiveCharts;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
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
using System.Windows.Shapes;
using InfluxDB.Client;
using InfluxDB.Client.Api.Domain;
using InfluxDB.Client.Core;
using InfluxDB.Client.Writes;
using System.Threading;
using System.Net;

namespace Appli_CocoriCO2
{
    /// <summary>
    /// Logique d'interaction pour ComDebugWindow.xaml
    /// </summary>
    public partial class ComDebugWindow : Window
    {
        MainWindow MW = ((MainWindow)Application.Current.MainWindow);
        DateTime lastFileWrite = DateTime.Now.ToUniversalTime();

        string token = Properties.Settings.Default["InfluxDBToken"].ToString();
        string bucket = Properties.Settings.Default["InfluxDBBucket"].ToString();
        string org = Properties.Settings.Default["InfluxDBOrg"].ToString();

        InfluxDBClient client;
        CancellationTokenSource cts = new CancellationTokenSource();

        public ComDebugWindow()
        {
            InitializeComponent();
#pragma warning disable CS4014 // Dans la mesure où cet appel n'est pas attendu, l'exécution de la méthode actuelle continue avant la fin de l'appel. Envisagez d'appliquer l'opérateur 'await' au résultat de l'appel.
            InitializeAsync();
#pragma warning restore CS4014 // Dans la mesure où cet appel n'est pas attendu, l'exécution de la méthode actuelle continue avant la fin de l'appel. Envisagez d'appliquer l'opérateur 'await' au résultat de l'appel.

            client = InfluxDBClientFactory.Create("http://localhost:8086", token.ToCharArray());
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
            var dueTime = TimeSpan.FromMinutes(t);
            var interval = TimeSpan.FromMinutes(t);

            // TODO: Add a CancellationTokenSource and supply the token here instead of None.
            await RunPeriodicAsync(saveData, dueTime, interval, cts.Token);
        }

        private void tb2_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (tb2.Text.Length > 0)
            {
                ReadData(tb2.Text);
            }
        }



        public void ReadData(string data)
        {

            try
            {
                Condition c = JsonConvert.DeserializeObject<Condition>(data);
                c.lastUpdated = new DateTime(1970, 1, 1, 0, 0, 0, 0, System.DateTimeKind.Utc).AddSeconds(c.time);
                MW.conditionData.Add(c);



                MW.statusLabel1.Text = "Last updated: " + c.lastUpdated.ToString() + " UTC";
                if (c.command == 2 && (c.regulpH != null)) //SEND_PARAMS
                {
                    MW.conditions[c.condID].lastUpdated = c.lastUpdated;
                    
                   // MW.Labels[MW.Labels.Length-1] = c.lastUpdated.ToString();
                    //MW.seriesCollection[0].Values.Add(c.command);

                    double sortie = MW.conditions[c.condID].regulpH.sortiePID_pc;
                    MW.conditions[c.condID].regulpH = c.regulpH;
                    //MW.conditions[c.condID].regulpH.consigne = consigne;
                    MW.conditions[c.condID].regulpH.sortiePID_pc = sortie;

                    sortie = MW.conditions[c.condID].regulTemp.sortiePID_pc;
                    MW.conditions[c.condID].regulTemp = c.regulTemp;
                   // MW.conditions[c.condID].regulTemp.consigne = consigne;
                    MW.conditions[c.condID].regulTemp.sortiePID_pc = sortie;
                }
                else if (c.command == 3  && (c.Meso != null))
                {
                    ///MW.monitoringWindow.Labels.Add(c.lastUpdated.ToString());

                    for (int i = 0; i < 3; i++) {
                        MW.conditions[c.condID].Meso[i] = c.Meso[i];
                        if (MW.conditions[c.condID].Meso[i].debit < 0) MW.conditions[c.condID].Meso[i].debit = 0;
                    }
                    MW.conditions[c.condID].temperature = c.temperature;
                    MW.conditions[c.condID].pH = c.pH;
                    MW.conditions[c.condID].regulpH.sortiePID_pc = c.regulpH.sortiePID_pc;
                    
                    if(c.condID != 0)
                    {
                        MW.conditions[c.condID].regulpH.consigne = c.regulpH.consigne;
                        MW.conditions[c.condID].regulTemp.sortiePID_pc = c.regulTemp.sortiePID_pc;
                        MW.conditions[c.condID].regulTemp.consigne = c.regulTemp.consigne;
                    }
                }
                else if (c.command == 6)
                {
                    MW.ambiantConditions = JsonConvert.DeserializeObject<Ambiant>(data);
                    MW.ambiantConditions.lastUpdated = new DateTime(1970, 1, 1, 0, 0, 0, 0, System.DateTimeKind.Utc).AddSeconds(MW.ambiantConditions.time);
                    MW.statusLabel1.Text = "Last updated: " + MW.ambiantConditions.lastUpdated.ToString() + " UTC";
                }
                else if (c.command == 8)
                {

                    MW.masterParams = JsonConvert.DeserializeObject<MasterParams>(data);
                }

                MW.DisplayData(c.command);
            }
#pragma warning disable CS0168 // La variable 'e' est déclarée, mais jamais utilisée
            catch (Exception e)
#pragma warning restore CS0168 // La variable 'e' est déclarée, mais jamais utilisée
            {

            }
        }

        private void saveData()
        {
            try
            {
                Condition c = MW.conditionData.Last<Condition>();
                if (c.lastUpdated != lastFileWrite)
                {
                    DateTime dt = DateTime.Now.ToUniversalTime();
                    string filePath = Properties.Settings.Default["dataFileBasePath"].ToString()+ "_"+dt.ToString("yyyy-MM-dd") + ".csv";
                    filePath = filePath.Replace('\\', '/');

                    saveToFile(filePath, dt);
                    //if (c.lastUpdated.Day != lastFileWrite.Day) ftpTransfer(filePath);
                    if (c.lastUpdated.Hour != lastFileWrite.Hour)// POur tester
                    {
                        ftpTransfer(filePath);
                        lastFileWrite = c.lastUpdated;
                    }
                    MW.conditionData.Clear();
                }
            }catch(Exception e)
            {
                MessageBox.Show("Error writing data: " + e.Message, "Error saving data");
            }
            
        }

        private async Task writeDataPointAsync(int conditionId, int MesoID, string field, double value, DateTime dt)
        {
            string tag; 
            if (MesoID == -1) tag = "AmbientData";
            else tag = MesoID.ToString();
            var point = PointData
              .Measurement("CRCBN")
              .Tag("Condition", conditionId.ToString())
              .Tag("Mesocosm", tag)
              .Field(field, value)
              .Timestamp(dt.ToUniversalTime(), WritePrecision.S);

            try
            {
                var writeApi = client.GetWriteApiAsync();
                await writeApi.WritePointAsync(bucket, org, point);
            
            }catch(Exception e)
            {

            }

}


        private void saveToFile(string filePath, DateTime dt)
        {
            if (!System.IO.File.Exists(filePath))
            {
                //Write headers
                String header = "Time;Sun;Tide;Ambient_O2;Ambient_Conductivity;Ambient_Salinity;Ambient_Turbidity;Ambient_Fluo;Ambient_Temperature;Ambient_pH;Cold_Water_Pressure;Hot_Water_Pressure;";

                for (int i = 0; i < 4; i++)
                {
                    header += "Condition["; header += i; header += "]_Temperature;";
                    header += "Condition["; header += i; header += "]_pH;";
                    header += "Condition["; header += i; header += "]_consigne_pH;";
                    header += "Condition["; header += i; header += "]_sortiePID_pH;";
                    if (i > 0)
                    {
                        header += "Condition["; header += i; header += "]_consigne_Temperature;";
                        header += "Condition["; header += i; header += "]_sortiePID_Temperature;";
                    }
                    for (int j = 0; j < 3; j++)
                    {
                        header += "Condition["; header += i; header += "]_Meso["; header += j; header += "]_Temperature;"; 
                        header += "Condition["; header += i; header += "]_Meso["; header += j; header += "]_pH;"; 
                        header += "Condition["; header += i; header += "]_Meso["; header += j; header += "]_FlowRate;"; 
                        header += "Condition["; header += i; header += "]_Meso["; header += j; header += "]_LevelH;"; 
                        header += "Condition["; header += i; header += "]_Meso["; header += j; header += "]_LevelL;"; 
                        header += "Condition["; header += i; header += "]_Meso["; header += j; header += "]_LevelLL;"; 
                    }
                }
                header += "\n";
                System.IO.File.WriteAllText(filePath, header);
            }

            string data = dt.ToString(); ; data += ";";

            double sun = MW.ambiantConditions.sun ? 1 : 0;
            double tide = MW.ambiantConditions.tide ? 1 : 0;

            data += sun; data += ";";
            data += MW.ambiantConditions.tide ? 1 : 0; data += ";";
            data += MW.ambiantConditions.oxy; data += ";";
            data += MW.ambiantConditions.cond; data += ";";
            data += MW.ambiantConditions.salinite; data += ";";
            data += MW.ambiantConditions.turb; data += ";";
            data += MW.ambiantConditions.fluo; data += ";";
            data += MW.ambiantConditions.temperature; data += ";";
            data += MW.ambiantConditions.pH; data += ";";
            data += MW.ambiantConditions.pressionEA; data += ";";
            data += MW.ambiantConditions.pressionEC; data += ";";

            writeDataPointAsync(0, -1, "sun", sun, dt);
            writeDataPointAsync(0, -1, "tide", tide, dt);
            writeDataPointAsync(0, -1, "oxy", MW.ambiantConditions.oxy, dt);
            writeDataPointAsync(0, -1, "conductivity", MW.ambiantConditions.cond, dt);
            writeDataPointAsync(0, -1, "salinity", MW.ambiantConditions.salinite, dt);
            writeDataPointAsync(0, -1, "turb", MW.ambiantConditions.turb, dt);
            writeDataPointAsync(0, -1, "fluo", MW.ambiantConditions.fluo, dt);
            writeDataPointAsync(0, -1, "temperature", MW.ambiantConditions.temperature, dt);
            writeDataPointAsync(0, -1, "pH", MW.ambiantConditions.pH, dt);

            writeDataPointAsync(0, -1, "Cold_Water_Pressure", MW.ambiantConditions.pressionEA, dt);
            writeDataPointAsync(0, -1, "Hot_Water_Pressure", MW.ambiantConditions.pressionEC, dt);

            for (int i = 0; i < 4; i++)
            {
                data += MW.conditions[i].temperature; data += ";";
                data += MW.conditions[i].pH; data += ";";
                data += MW.conditions[i].regulpH.consigne; data += ";";
                data += MW.conditions[i].regulpH.sortiePID_pc; data += ";";

                writeDataPointAsync(i, -1, "temperature", MW.conditions[i].temperature, dt);
                writeDataPointAsync(i, -1, "pH", MW.conditions[i].pH, dt);
                writeDataPointAsync(i, -1, "regulpH.consigne", MW.conditions[i].regulpH.consigne, dt);
                writeDataPointAsync(i, -1, "regulpH.sortiePID", MW.conditions[i].regulpH.sortiePID_pc, dt);

                if (i > 0)
                {
                    data += MW.conditions[i].regulTemp.consigne; data += ";";
                    data += MW.conditions[i].regulTemp.sortiePID_pc; data += ";";
                    writeDataPointAsync(i, -1, "regulTemp.consigne", MW.conditions[i].regulTemp.consigne, dt);
                    writeDataPointAsync(i, -1, "regulTemp.sortiePID", MW.conditions[i].regulTemp.sortiePID_pc, dt);
                }

                for (int j = 0; j < 3; j++)
                {
                    double LH = MW.conditions[i].Meso[j].alarmeNiveauHaut ? 1 : 0;
                    double LL = MW.conditions[i].Meso[j].alarmeNiveauBas ? 1 : 0;
                    double LLL = MW.conditions[i].Meso[j].alarmeNiveauTresBas ? 1 : 0;

                    data += MW.conditions[i].Meso[j].temperature; data += ";";
                    data += MW.conditions[i].Meso[j].pH; data += ";";
                    data += MW.conditions[i].Meso[j].debit; data += ";";
                    data += LH; data += ";";
                    data += LL; data += ";";
                    data += LLL; data += ";";

                    writeDataPointAsync(i, j, "temperature", MW.conditions[i].Meso[j].temperature, dt);
                    writeDataPointAsync(i, j, "pH", MW.conditions[i].Meso[j].pH, dt);
                    writeDataPointAsync(i, j, "debit", MW.conditions[i].Meso[j].debit, dt);
                    writeDataPointAsync(i, j, "LH", LH, dt);
                    writeDataPointAsync(i, j, "LL", LL, dt);
                    writeDataPointAsync(i, j, "LLL", LLL, dt);

                }
            }
            data += "\n";
            System.IO.File.AppendAllText(filePath, data);
        }

        private void ftpTransfer(string fileName)
        {
            string ftpUsername = Properties.Settings.Default["ftpUsername"].ToString();
            string ftpPassword = Properties.Settings.Default["ftpPassword"].ToString();
            string ftpDir= "ftp://"+Properties.Settings.Default["ftpDir"].ToString();

            string fn = fileName.Substring(fileName.LastIndexOf('/')+1);
            ftpDir += fn;
            using (var client = new WebClient())
            {
                client.Credentials = new NetworkCredential(ftpUsername, ftpPassword);
                client.UploadFile(ftpDir, WebRequestMethods.Ftp.UploadFile, fileName);
            }
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            this.Hide();
            e.Cancel = true;
        }
    }

}

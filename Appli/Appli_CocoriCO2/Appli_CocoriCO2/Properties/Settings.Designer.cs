﻿//------------------------------------------------------------------------------
// <auto-generated>
//     Ce code a été généré par un outil.
//     Version du runtime :4.0.30319.42000
//
//     Les modifications apportées à ce fichier peuvent provoquer un comportement incorrect et seront perdues si
//     le code est régénéré.
// </auto-generated>
//------------------------------------------------------------------------------

namespace Appli_CocoriCO2.Properties {
    
    
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.Editors.SettingsDesigner.SettingsSingleFileGenerator", "16.8.1.0")]
    internal sealed partial class Settings : global::System.Configuration.ApplicationSettingsBase {
        
        private static Settings defaultInstance = ((Settings)(global::System.Configuration.ApplicationSettingsBase.Synchronized(new Settings())));
        
        public static Settings Default {
            get {
                return defaultInstance;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("192.168.1.1")]
        public string MasterIPAddress {
            get {
                return ((string)(this["MasterIPAddress"]));
            }
            set {
                this["MasterIPAddress"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("10")]
        public int dataQueryInterval {
            get {
                return ((int)(this["dataQueryInterval"]));
            }
            set {
                this["dataQueryInterval"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("1")]
        public string dataLogInterval {
            get {
                return ((string)(this["dataLogInterval"]));
            }
            set {
                this["dataLogInterval"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("C:/CocoriCO2/CocoriCO2_CRCBN")]
        public string dataFileBasePath {
            get {
                return ((string)(this["dataFileBasePath"]));
            }
            set {
                this["dataFileBasePath"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("")]
        public string ftpUsername {
            get {
                return ((string)(this["ftpUsername"]));
            }
            set {
                this["ftpUsername"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("")]
        public string ftpPassword {
            get {
                return ((string)(this["ftpPassword"]));
            }
            set {
                this["ftpPassword"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("")]
        public string ftpDir {
            get {
                return ((string)(this["ftpDir"]));
            }
            set {
                this["ftpDir"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("http://localhost:8086/orgs/de9be6257a0a3477/dashboards/077b31f0db0d2000?lower=now" +
            "%28%29%20-%207d")]
        public string InfluxDBWebpage {
            get {
                return ((string)(this["InfluxDBWebpage"]));
            }
            set {
                this["InfluxDBWebpage"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("eUNDgxqrwAFsb6pjOdqGkW-Qr9NxorO5WQamvvgtquXFJgUz1tyG2Ee-G7HhI_IKk9L3JavnAoxrMZ5ev" +
            "Ls9Ig==")]
        public string InfluxDBToken {
            get {
                return ((string)(this["InfluxDBToken"]));
            }
            set {
                this["InfluxDBToken"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("CocoriCO2")]
        public string InfluxDBBucket {
            get {
                return ((string)(this["InfluxDBBucket"]));
            }
            set {
                this["InfluxDBBucket"] = value;
            }
        }
        
        [global::System.Configuration.UserScopedSettingAttribute()]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [global::System.Configuration.DefaultSettingValueAttribute("CNRS")]
        public string InfluxDBOrg {
            get {
                return ((string)(this["InfluxDBOrg"]));
            }
            set {
                this["InfluxDBOrg"] = value;
            }
        }
    }
}

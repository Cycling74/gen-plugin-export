const maxApi = require("max-api");
const fs = require("fs");
const xml = require("xmldom");
const exec = require("child_process");
const DICT_ID = "args";

// Public methods
maxApi.addHandlers({
    bang: async () => {
        let args = await maxApi.getDict(DICT_ID);
        let currentdir = process.cwd();
        let jucername = "";

        maxApi.post(currentdir);
        maxApi.post(args);
        
        if (args.name === "") {
            args.name = "C74-Gen-" + args.type + "Plugin";
        }
        
        switch (args.type) {
            case "VST3":
                jucername = "C74-Gen-VST3Plugin.jucer";
                break;
            case "AU":
                jucername = "C74-Gen-AUPlugin.jucer";
                break;
            case "iOS":
                jucername = "C74-Gen-Application.jucer";
                break;
            default:
                maxApi.post("Invalid export type");
                break;
        }

        // Grab the .jucer file which we can traverse and edit
        let jucerfile = currentdir + "/Projucer/" + jucername;
        maxApi.post("Using .jucer file: " + jucerfile);
        let xmlfile = fs.readFileSync(jucerfile, 'utf8');
        let xmldoc = new xml.DOMParser().parseFromString(xmlfile);

        // JUCERPROJECT
        let juceproject = xmldoc.getElementsByTagName("JUCERPROJECT")[0];
        juceproject.setAttribute("name", args.name);
        juceproject.setAttribute("bundleIdentifier", "com.cycling74." + args.name);
        juceproject.setAttribute("pluginName", args.name);
        juceproject.setAttribute("pluginAUExportPrefix", args.name + "AU");
        juceproject.setAttribute("aaxIdentifier", "com.cycling74." + args.name);
        juceproject.setAttribute("pluginChannelConfigs", args.channelconf);

        // MAINGROUP
        let maingroup = juceproject.getElementsByTagName("MAINGROUP")[0];
        maingroup.setAttribute("name", args.name);

        // EXPORTFORMATS
        let exportformats = juceproject.getElementsByTagName("EXPORTFORMATS")[0];
        let xcode_mac = exportformats.getElementsByTagName("XCODE_MAC");
        if (xcode_mac[0]) {
            let conf = xcode_mac[0].getElementsByTagName("CONFIGURATION");
            if (conf[0]) {
                conf[0].setAttribute("targetName", args.name);
            }
        }

        let xcode_iphone = exportformats.getElementsByTagName("XCODE_IPHONE");
        if (xcode_iphone[0]) {
            let conf = xcode_iphone[0].getElementsByTagName("CONFIGURATION");
            if (conf[0]) {
                conf[0].setAttribute("targetName", args.name);
            }
        }

        let vs2019 = exportformats.getElementsByTagName("VS2019");
        if (vs2019[0]) {
            let conf = vs2019[0].getElementsByTagName("CONFIGURATION");
            if (conf[0]) {
                conf[0].setAttribute("targetName", args.name);
            }
        }

        // Write XML back to a file
        let newJucer = currentdir + "/Projucer/out-" + jucername;
        fs.writeFileSync(newJucer, new xml.XMLSerializer().serializeToString(xmldoc));
        maxApi.post("New .jucer file written to " + newJucer);

        // Run the proper JUCER application to generate Xcode or VS projects.
        if (process.platform === "darwin") {
            // Generate the Xcode project
            let cmd = "open -n \"" + currentdir + "/Projucer/Projucer.app\" --args --resave \"" + newJucer + "\"";
            maxApi.post(cmd);
            exec.exec(cmd);

            // Try to build the project
            try {
                let project = currentdir + "/" + args.type + "-Builds/MacOSX/" + args.name + ".xcodeproj";
                let buildcmd = "xcodebuild -project \"" + project + "\" -configuration " + args.configuation;
                maxApi.post(buildcmd);
                exec.exec(buildcmd);
            } catch (error) {
                maxApi.post(error);
            }
        }
        else if (process.platform === "win32") {
            // Generate the VS2019 project
            newJucer = newJucer.replace("/", "\\");
            let cmd = '"' + currentdir + "\\Projucer\\Projucer.exe\" --resave \"" + newJucer + "\"";
            maxApi.post(cmd);
            exec.exec(cmd);

            // Just open the explorer to show the Visual Studio Solution
            try {
                let slndir = currentdir + "\\" + args.type + "-Builds\\VisualStudio2019\\";
                let cmd = "explorer " + slndir;
                maxApi.post(cmd);
                exec.exec(cmd);
            } catch (error) {
                maxApi.post(error);
            }
        }
    }
});

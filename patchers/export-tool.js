const maxApi = require("max-api");
const exec = require("child_process");


maxApi.addHandlers({
    bang: async () => {
        let cmake = "";
        if (process.platform === "darwin") {
            cmake = "export PATH=$PATH:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin && cmake";
        } else {
            cmake = "C:\\CMake\\bin\\cmake.exe";
        }

        // Verify CMake exists
        try {
            let cmd = cmake + " --version";
            exec.exec(cmd, (error, stdout, stderr) => {
                let expected = "cmake version ";
                if (stdout.length > expected.length) {
                    if (stdout.slice(0, expected.length) !== expected) {
                        throw "CMake not found";
                    }
                }
            });
        } catch (error) {
            if (process.platform === "darwin") {
                maxApi.post("CMake was not found in the path: /usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin", maxApi.POST_LEVEL.ERROR);
            } else {
                maxApi.post("CMake was not found at C:\\CMake\\bin\\cmake.exe", maxApi.POST_LEVEL.ERROR);
            }
            return;
        }

        // Now run CMake in release mode
        try {
            let cmd = "cd ../misc/build && " + cmake + " -DCMAKE_BUILD_TYPE=Release .. && " + cmake + "--build .";
            exec.exec(cmd, (error, stdout, stderr) => {
                maxApi.post(stdout);
            });
        } catch (error) {
            maxApi.post("Failure to run CMake", maxApi.POST_LEVEL.ERROR);
        }
    }
});
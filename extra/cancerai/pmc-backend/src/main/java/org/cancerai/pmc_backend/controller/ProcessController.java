package org.cancerai.pmc_backend.controller;

import org.cancerai.pmc_backend.entility.ProcessEntility;
import org.springframework.web.bind.annotation.CrossOrigin;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.reactive.function.client.WebClient;

import java.util.ArrayList;

@CrossOrigin(origins = "*")
@RestController()
public class ProcessController {
    @GetMapping("/get-process")
    public ArrayList<ProcessEntility> cleanProcess() {
        ArrayList<String> arr = getProcessList();
        ArrayList<ProcessEntility> result = new ArrayList<>();
        final String DEAD = "Dead";

//        exclude Dead lines
        for (String i : arr) {
            String[] item = i.split("\t");
            String pid = item[0];
            String status = item[1];
            String cmd = item[2];

            if (DEAD.equals(status)) {
                continue;
            }
            result.add(new ProcessEntility(pid, status, cmd));
        }

        return result;
    }

    private static ArrayList<String> getProcessList() {
        ArrayList<String> array = new ArrayList<>();
        WebClient webClient = WebClient.create("http://127.0.0.1:8012");

        String response = webClient.post()
                .uri("/api/list")
                .retrieve()
                .bodyToMono(String.class)
                .block();

        String[] splitRes = response.split("\n");
        for (String item : splitRes) {
            array.add(item);
        }
        return array;
    }
}

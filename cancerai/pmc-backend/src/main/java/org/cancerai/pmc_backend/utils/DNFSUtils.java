package org.cancerai.pmc_backend.utils;

import org.cancerai.pmc_backend.entility.Note;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.http.*;
import org.springframework.web.client.RestTemplate;

import java.util.ArrayList;
import java.util.HashMap;

public class DNFSUtils {
    private final static String domain = "http://qsont.xyz:8006";

    public static ArrayList<Note> getNodes(Integer pid) {
        RestTemplate restTemplate = new RestTemplate();
//        prepare request body
        HashMap<String, Integer> requestBody = new HashMap<>();
        requestBody.put("pid", pid);
//        prepare request header
        HttpHeaders requestHeader = new HttpHeaders();
        requestHeader.setContentType(MediaType.APPLICATION_JSON);

        ResponseEntity<ArrayList<Note>> response = restTemplate.exchange(
                domain + "/get_notes",
                HttpMethod.POST,
                new HttpEntity<>(requestBody, requestHeader),
                new ParameterizedTypeReference<ArrayList<Note>>() {
                }
        );

        return response.getBody();
    }

    public static HashMap<String, String> addNote(int pid, String title, String content) {
        RestTemplate restTemplate = new RestTemplate();
        HashMap<String, String> body = new HashMap<>();
        HttpHeaders header = new HttpHeaders();

        header.setContentType(MediaType.APPLICATION_JSON);
        body.put("pid", String.valueOf(pid));
        body.put("title", title);
        body.put("content", content);

        return restTemplate.exchange(
                domain + "/add_note",
                HttpMethod.POST,
                new HttpEntity<>(body, header),
                new ParameterizedTypeReference<HashMap<String, String>>() {
                }
        ).getBody();
    }

    public static HashMap<String, String> deleteNote(int id) {
        RestTemplate restTemplate = new RestTemplate();
        HashMap<String, String> body = new HashMap<>();
        HttpHeaders header = new HttpHeaders();

        header.setContentType(MediaType.APPLICATION_JSON);
        body.put("id", String.valueOf(id));

        return restTemplate.exchange(
                domain + "/delete_note",
                HttpMethod.POST,
                new HttpEntity<>(body, header),
                new ParameterizedTypeReference<HashMap<String, String>>() {
                }
        ).getBody();
    }

    public static HashMap<String, String> updateNote(int id, int pid, String title, String content) {
        RestTemplate restTemplate = new RestTemplate();
        HashMap<String, String> body = new HashMap<>();
        HttpHeaders header = new HttpHeaders();

        header.setContentType(MediaType.APPLICATION_JSON);
        body.put("id", String.valueOf(id));
        body.put("pid", String.valueOf(pid));
        body.put("title", title);
        body.put("content", content);

        return restTemplate.exchange(
                domain + "/update_note",
                HttpMethod.POST,
                new HttpEntity<>(body, header),
                new ParameterizedTypeReference<HashMap<String, String>>() {
                }
        ).getBody();
    }

    public static ArrayList<HashMap<String, String>> parseUserInfo(ArrayList<String> userLIst) {
        ArrayList<HashMap<String, String>> array = new ArrayList<>();

        for (String s : userLIst) {
            HashMap<String, String> map = new HashMap<>();
            String[] splitArray = s.split("::");
            map.put("id", splitArray[0]);
            map.put("name", splitArray[1]);
            map.put("password", splitArray[2]);
            map.put("role", splitArray[3]);
            array.add(map);
        }

        return array;
    }
}

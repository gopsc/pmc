package org.cancerai.pmc_backend.controller;

import jakarta.servlet.http.HttpServletResponse;
import org.cancerai.pmc_backend.entility.Note;
import org.cancerai.pmc_backend.utils.DNFSUtils;
import org.cancerai.pmc_backend.utils.HttpResponseText;
import org.cancerai.pmc_backend.vo.Result;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;

@CrossOrigin(origins = "*")
@RestController
@RequestMapping("/api")
public class DNFSController {
    @GetMapping("/get-notes")
    public static Result getNotes(@RequestParam int pid) {
        return Result.success(HttpResponseText.SUCCESS_TO_REQUEST, DNFSUtils.getNodes(pid));
    }

    @PutMapping("/add-note")
    public Result addNote(
            @RequestParam int pid,
            @RequestParam String title,
            @RequestParam String content,
            HttpServletResponse response
    ) {
        HashMap<String, String> re = DNFSUtils.addNote(pid, title, content);
        String value = re.get("message");

        if ("Note added successfully".equals(value)) {
            return Result.success(HttpResponseText.SUCCESS_TO_ADD_NOTE);
        } else {
            response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
            return Result.error(HttpResponseText.INTERNAL_SERVER_ERROR);
        }
    }

    @DeleteMapping("/delete-note")
    public Result deleteNote(
            @RequestParam int id,
            HttpServletResponse response
            ) {
        HashMap<String, String> re = DNFSUtils.deleteNote(id);
        String value = re.get("message");

        if ("Note deleted successfully".equals(value)) {
            return Result.success(HttpResponseText.SUCCESS_TO_DELETE_NOTE);
        } else {
            response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
            return Result.error(HttpResponseText.INTERNAL_SERVER_ERROR);
        }
    }

    @PatchMapping("/update-note")
    public static Result updateNote(
            @RequestBody Note note,
            HttpServletResponse response
            ) {
        int id = note.getId();
        int pid = note.getPid();
        String title = note.getTitle();
        String content = note.getContent();

        HashMap<String, String> re = DNFSUtils.updateNote(id, pid, title, content);
        String value = re.get("message");
        if ("Your change is up-to-date.".equals(value)) {
            return Result.success(HttpResponseText.SUCCESS_TO_UPDATE_NOTE);
        } else {
            response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
            return Result.error(HttpResponseText.INTERNAL_SERVER_ERROR);
        }
    }
}

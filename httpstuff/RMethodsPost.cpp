/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RMethodsPost.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ahajji <ahajji@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/03 00:00:05 by ahajji            #+#    #+#             */
/*   Updated: 2024/04/03 21:50:21 by ahajji           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestMethods.hpp"

std::string getLastPart(const std::string& path) {
    size_t pos = path.find_last_of("/");
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return "";
}

std::string listFils(const std::string& path, Request request)
{
    std::string html = "<html><body><ul>";
    DIR* dirp = opendir(path.c_str());
    if (dirp) {
        struct dirent * dp;
        while ((dp = readdir(dirp)) != NULL) {
            std::string filename(dp->d_name);
            html += "<li><a href=\"" + request.getRequestRessource() + filename + "\">" + filename + "</a></li>";
        }
        closedir(dirp);
    }
    html += "</ul></body></html>";
    std::cout << path << "\n\n\n\n";
    return html;
}

std::string returnContentFile(std::string path, Response& response)
{
    std::ifstream file;
    file.open(path.c_str(), std::ios::binary);
    if (!file || !file.is_open()) {
        response.buildResponse(404);
        std::cerr << "Unable to open file\n";
        return "error file";
    }
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    std::string fileContent(buffer.begin(), buffer.end());
    return fileContent;
}

void  returnDefaultContentFile(Request& request, DataConfig config, std::string path, Response& response)
{
    std::string line;
    std::string nameFile;
    CgiOutput  data;;
       
    // if((config.getSpecificLocation(request.getLocation())->autoIndex == 1 || config.getAutoIndex() == 1))
    std::cout << "this is ok soooooo "<< request.getLocation() << std::endl;
    // std::cout <<"ahahahahaahah morrrrrrrraddddddd " << config.getSpecificLocation(request.getLocation())->autoIndex << std::endl;
    if(request.getBody() == "fdf")
    {
        std::cout << path << std::endl;
        std::cout << response.getContentLength() << std::endl;
        std::cout << request.getBody();
        std::cout << config.getAutoIndex();
        // response.setStatus(200);
        // response.setContentType("index.html");
        // response.setContentLength(listFils(path, request).size());
        // response.setResponseBody(listFils(path, request));
        // response.buildResponse(200);
        // return ;
    }
    // if(config.getSpecificLocation(request.getLocation())->index.empty() == 0)
    //     nameFile = config.getSpecificLocation(request.getLocation())->index;
    // else if(config.getIndex().empty() == 0)
    //     nameFile = config.getIndex();
    // path += nameFile;
    // if (nameFile.empty())
    // {
    //     response.buildResponse(403);
    //     return ;
    // }
    // std::string extension = "";
    // std::size_t pos = nameFile.find_last_of(".");
    // if (pos != std::string::npos)
    //     extension = nameFile.substr(pos);
    // std::string fileContent;
    // if (extension == ".php")
    // {   
    //     if(config.getSpecificLocation(request.getLocation())->cgiExtension != "")
    //     {
    //         data = Cgi::CallCgi(path, request, "/", config);
    //         std::cout <<"hiii    " << data.getBody() << "hiiiiiiiiii am her \n\n\n\n\n\n\n\n";
    //         if(data.getCgiError() == "error")
    //             response.buildResponse(500);
    //         else if(data.getCgiError() == "time out")
    //             response.buildResponse(504);
    //         else if(data.getLocation().empty())
    //         {
    //             response.setContentType(getLastPart(path));
    //             response.setContentLength(data.getBody().size());
    //             response.setResponseBody(data.getBody());
    //             response.buildResponse(200);
    //         }
    //         else
    //         {
    //             std::cout << "hi i am her " << data.getLocation() << "\n";
    //             response.setHeader("Location:", data.getLocation());
    //             response.buildResponse(307);
    //         }
    //     }
    //     else
    //         response.buildResponse(403);
    // }
    // else
    // {
    //      std::cout << "hahahahahahahahahaha   \n\n\n\n ";
    //     fileContent = returnContentFile(path, response);
    //     if (fileContent == "error file")
    //         return ;
    //     response.setStatus(200);
    //     response.setContentType(getLastPart(path));
    //     response.setContentLength(fileContent.size());
    //     response.setResponseBody(fileContent);
    //     response.buildResponse(200);
    // }
}

void returnSpecificContentFile(std::string path, DataConfig config,Response& response, Request& request)
{
    std::ifstream file;
    file.open(path.c_str(), std::ios::binary);
    if((config.getSpecificLocation(request.getLocation())->autoIndex == 1 || config.getAutoIndex() == 1))
    {
        response.setStatus(200);
        response.setContentType("index.html");
        response.setContentLength(listFils(path, request).size());
        response.setResponseBody(listFils(path, request));
        response.buildResponse(200);
        return ;
    }
    if (!file || !file.is_open()) {
        response.buildResponse(404);
        std::cerr << "Unable to open file\n";
        return;
    }
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    response.setStatus(200);
    response.setContentType(getLastPart(path));
    response.setContentLength(buffer.size());
    std::string fileContent(buffer.begin(), buffer.end());
    response.setResponseBody(fileContent);
    response.buildResponse(200);
}
Response RequestMethod::POST(Request& request, DataConfig config)
{   
    std::string path = request.getPath();
    Response response;
    std::string content;
    std::string fileContent;
    if (path[path.size() - 1] == '/')
    {
        std::cout << "hi iam her okkkbroooooo\n\n\n\n\n\n\n";
        returnDefaultContentFile(request, config, path, response);
        
    }
    else
        returnSpecificContentFile(path, config, response, request);
    return response;
}
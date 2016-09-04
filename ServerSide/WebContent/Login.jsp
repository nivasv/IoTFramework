<%@ page language="java" contentType="text/html; charset=ISO-8859-1"
    pageEncoding="ISO-8859-1"%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
        <TITLE>Login using jsp</TITLE>
        <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">

    </head>

 

    <BODY background="http://cdn.c3iot.com/wp-content/uploads/2015/12/14Ways1.jpg">

        <H1 style="color:white" >Administrator Login</H1>

        <%

        String myname =  (String)session.getAttribute("username");

        

        if(myname!=null)

            {

             out.println("Welcome  "+myname+"  , <a href=\"test.jsp\" ></a>");

            }

        else 

            {

            %>

            <form action="checkLogin.jsp">

                <table style="color:white">

                    <tr>

                        <td> Username  : </td><td> <input name="username" size=15 type="text" /> </td> 

                    </tr>

                    <tr>

                        <td> Password  : </td><td> <input name="password" size=15 type="password" /> </td> 

                    </tr>

                </table>
                

                <input type="submit" class="btn-primary"  value="login" />

            </form>

            <% 

            }

         

             

            %>

         

    </BODY>

</HTML> 


























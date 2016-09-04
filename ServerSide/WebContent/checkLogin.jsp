<%@ page language="java" contentType="text/html; charset=ISO-8859-1"
    pageEncoding="ISO-8859-1"%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">

<title>Insert title here</title>
</head>

<body>

<%

            String username = request.getParameter("username");

            String password = request.getParameter("password");

           

            if (!(username.toLowerCase().trim().equals("admin")) && !(password.toLowerCase().trim().equals("admin")))
            {
            	//response.sendRedirect("Login.jsp");
%>
            	<script type="text/javascript">
        	alert("Kindly Re-enter User Name & Password");
        	window.location.href = "Login.jsp";
        	</script> 
<% 
            }

 %>

            

      <%       if (username.toLowerCase().trim().equals("admin") && password.toLowerCase().trim().equals("admin")) {
            	
    	  
          %>  	
          
          <%
          response.sendRedirect("test.jsp");

              out.println("Welcome " + username + " <a href=\"test.jsp\"></a>");

                session.setAttribute("username", username);

            }

           else 

               {
%>
        	  
        	<script type="text/javascript">
        	alert("Kindly Re-enter User Name & Password");
        	window.location.href = "Login.jsp";
        	</script>
           <% 	
           	
           
           }

 

 

 

 

%> 

<script type="text/javascript"> window.onload = alert; </script>



</body>
</html>
<%@ page language="java" contentType="text/html; charset=ISO-8859-1"
	pageEncoding="ISO-8859-1"%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<title>Insert title here</title>
</head>
<body
	background="http://edge.alluremedia.com.au/m/g/2016/03/shutterstock_277469792_1080.jpg">

	<%
		String username = (String) session.getAttribute("username");

		if (username != null)

		{
	%>
	<h1 style="color: white">Administrator Logged out</h1>

	<%
		out.println("<h1> <a href=\"Login.jsp\">Login</a></h1>");
			session.removeAttribute("username");

		}

		else

		{

			out.println("You are not logged in <a href=\"Login.jsp\">Login</a>");

		}
	%>

























</body>
</html>
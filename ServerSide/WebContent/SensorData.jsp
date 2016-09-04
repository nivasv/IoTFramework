<%@ page language="java" contentType="text/html; charset=ISO-8859-1"
	pageEncoding="ISO-8859-1" import="com.mongodb.MongoClient"
	import="com.mongodb.MongoException" import="com.mongodb.WriteConcern"
	import="com.mongodb.DB" import="com.mongodb.DBCollection"
	import="com.mongodb.BasicDBObject" import="com.mongodb.DBObject"
	import="com.mongodb.DBCursor" import="com.mongodb.Mongo"
	import="com.mongodb.ServerAddress" import="java.util.Arrays"
	import="java.io.*,java.util.*, javax.servlet.*"
	import="java.net.Inet4Address" import="java.net.InetAddress"
	import="java.net.InetSocketAddress" import="java.net.NetworkInterface"
	import="java.net.SocketException"%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<title>Sensor Details</title>

<script type="text/javascript">
	var refreshPeriod = 2000000; // 1 Seconds

	function refresh() {
		document.cookie = 'scrollTop=' + filterScrollTop();
		document.cookie = 'scrollLeft=' + filterScrollLeft();
		document.location.reload(true);
	}

	function getCookie(name) {
		var start = document.cookie.indexOf(name + "=");
		var len = start + name.length + 1;

		if (((!start) && (name != document.cookie.substring(0, name.length)))
				|| start == -1)
			return null;

		var end = document.cookie.indexOf(";", len);

		if (end == -1)
			end = document.cookie.length;

		return unescape(document.cookie.substring(len, end));
	}

	function deleteCookie(name) {
		document.cookie = name + "=" + ";expires=Thu, 01-Jan-1970 00:00:01 GMT";
	}

	function setupRefresh() {
		var scrollTop = getCookie("scrollTop");
		var scrollLeft = getCookie("scrollLeft");

		if (!isNaN(scrollTop)) {
			document.body.scrollTop = scrollTop;
			document.documentElement.scrollTop = scrollTop;
		}

		if (!isNaN(scrollLeft)) {
			document.body.scrollLeft = scrollLeft;
			document.documentElement.scrollLeft = scrollLeft;
		}

		deleteCookie("scrollTop");
		deleteCookie("scrollLeft");

		setTimeout("refresh()", refreshPeriod * 1000);
	}

	function filterResults(win, docEl, body) {
		var result = win ? win : 0;

		if (docEl && (!result || (result > docEl)))
			result = docEl;

		return body && (!result || (result > body)) ? body : result;
	}

	// Setting the cookie for vertical position
	function filterScrollTop() {
		var win = window.pageYOffset ? window.pageYOffset : 0;
		var docEl = document.documentElement ? document.documentElement.scrollTop
				: 0;
		var body = document.body ? document.body.scrollTop : 0;
		return filterResults(win, docEl, body);
	}

	// Setting the cookie for horizontal position
	function filterScrollLeft() {
		var win = window.pageXOffset ? window.pageXOffset : 0;
		var docEl = document.documentElement ? document.documentElement.scrollLeft
				: 0;
		var body = document.body ? document.body.scrollLeft : 0;
		return filterResults(win, docEl, body);
	}
</script>



</head>
<body onload="setupRefresh()" bgcolor="#DCDCDC">
	<button type="button" onclick="location = 'test.jsp'"
		style="float: left;">View All Data</button>
	&nbsp;
	<br>
	<br>
	<button type="button" onclick="location = 'Sensors.jsp'" style="float: left;">Connected Sensors</button>
	<br>
	<br>
	<h1 align="center" style="color: #1E90FF;">Customized Data</h1>
	<br>
	<br>
	<form action="SensorData.jsp">

		<table style="color: Blue">

			<tr>

				<td>Sensor_ID :</td>
				<td><input name="sid" size=20 type="text" /></td>

			</tr>
			
		</table>


		<input type="submit" class="btn-primary" value="Filter" />

	</form>


	<%
		//response.setIntHeader("Refresh", 1);
		//Date date = new Date();
		//out.print("<h2 align=\"center\">" + date.toString() + "</h2>");
		Mongo mongo = new Mongo("localhost", 27017);
		DB db = mongo.getDB("datastore");
		//out.println("Connect to database successfully");

		DBCollection coll = db.getCollection("sensordata");

		try {

					
			String sid=request.getParameter("sid");

			BasicDBObject getQuery = new BasicDBObject();

			getQuery.put("3",sid);
			

			DBCursor cursor = coll.find(getQuery);

			while (cursor.hasNext()) {
				out.println("<BR>");
				out.println(cursor.next());
			}
			out.print("<h2 align=\"right\">" + "No. of records:"
					+ cursor.count() + "</h2>");
		} catch (Exception e) {
			out.println("No Records Found");

		}
	%>




</body>
</html>
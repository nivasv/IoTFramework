/*******************************************************************************
 * Copyright (c) 2015 Institute for Pervasive Computing, ETH Zurich and others.
 * 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 * 
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.html.
 * 
 * Contributors:
 *    Matthias Kovatsch - creator and main architect
 *    Kai Hudalla (Bosch Software Innovations GmbH) - add endpoints for all IP addresses
 ******************************************************************************/
package src.main.java.org.eclipse.californium.examples;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.CoapServer;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.network.CoapEndpoint;
import org.eclipse.californium.core.network.config.NetworkConfig;
import org.eclipse.californium.core.server.resources.CoapExchange;

import com.mongodb.BasicDBList;
import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBCursor;
import com.mongodb.DBObject;
import com.mongodb.Mongo;
import com.mongodb.MongoClient;
import com.mongodb.MongoException;
import com.mongodb.WriteResult;
import com.mongodb.util.JSON;

import java.sql.Timestamp;
import java.util.Date;

public class DataStorage extends CoapServer {
	int count;

	private static final int COAP_PORT = NetworkConfig.getStandard().getInt(NetworkConfig.Keys.COAP_PORT);

	/*
	 * Application entry point.
	 */

	public class Sensor {
		String Value1;
		String Value2;
		String Value3;
		String Value4;
	};

	public static void main(String[] args) {

		try {

			// create server
			DataStorage server = new DataStorage();
			// add endpoints on all IP addresses
			server.addEndpoints();
			server.start();
			

		} catch (SocketException e) {
			System.err.println("Failed to initialize server: " + e.getMessage());
		}
	}

	/**
	 * Add endpoints listening on default CoAP port on all IP addresses of all
	 * network interfaces.
	 * 
	 * @throws SocketException
	 *             if network interfaces cannot be determined
	 */
	private void addEndpoints() throws SocketException {
		for (NetworkInterface ni : Collections.list(NetworkInterface.getNetworkInterfaces())) {
			for (InetAddress addr : Collections.list(ni.getInetAddresses())) {
				if (!addr.isLoopbackAddress() && addr instanceof Inet4Address) {
					InetSocketAddress bindToAddress = new InetSocketAddress(addr, COAP_PORT);
					addEndpoint(new CoapEndpoint(bindToAddress));
				}
			}
		}
	}

	/*
	 * Constructor for a new Hello-World server. Here, the resources of the
	 * server are initialized.
	 */
	public DataStorage() throws SocketException {

		// provide an instance of a Hello-World resource
		add(new HelloWorldResource());
	}

	/*
	 * Definition of the Hello-World Resource
	 */
	class HelloWorldResource extends CoapResource {

		public HelloWorldResource() {

			// set resource identifier
			super("Data");

			// set display name
			getAttributes().setTitle("Hello-World Resource");
		}

		@Override
		public void handleGET(CoapExchange exchange) {

			// respond to the request
			exchange.respond("Hello world for team b iot");
		}
		InetAddress addr;
		public void handlePOST(CoapExchange exchange) {
						
			addr=exchange.getSourceAddress();
			String addrs=addr.toString();
			System.out.println("Source address:"+addr);
			int port =exchange.getSourcePort();
			System.out.println("source port:"+port);
			exchange.accept();
			Date date= new Date();
	         //getTime() returns current time in milliseconds
		 long time = date.getTime();
	         //Passed the milliseconds to constructor of Timestamp class 
		 Timestamp ts = new Timestamp(time);
			//// int decimal = Integer.parseUnsignedInt(0x83, 16);
			//// ResponseCode rs=2;

			// System.out.println(exchange.getRequestText());

			String helchk = "Servercheck"; // Health Check
			if (exchange.getRequestText().equalsIgnoreCase(helchk)) {
				exchange.respond("Server Alive");
				exchange.respond(ResponseCode.VALID,"Server Alive");
			}
			String reg_tg = exchange.getRequestText(); // Registration Request
			String init_split[] = reg_tg.split(";"); //// in place of ; use ($,%) for delimiter for seperating other attributes other than data
			for (int i = 0; i < init_split.length; i++)
				System.out.println(init_split[i]); // checking the split action

			if (init_split[0].equalsIgnoreCase("registration"))
			{
				// System.out.println(exchange.getRequestText());
				
				
				Mongo mongo;
				try {
					mongo = new Mongo("localhost", 27017);
					DB db = mongo.getDB("regreq");
					DBCollection collection = db.getCollection("tokens");
					DBCursor cursor = (DBCursor) collection.findOne();
					exchange.respond(cursor.toString());
					
				} catch (UnknownHostException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (MongoException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				
				 				
				
				
				
				//exchange.respond(" Success 3984");
				// exchange.respond(ResponseCode.METHOD_NOT_ALLOWED,"Success
				// 3984");// validating the request
				// action

			}

			try {
				exchange.respond(ResponseCode.VALID, "Inserted Successfully");
				List<String> dlt = new ArrayList<String>();
				 for (int i = 0; i < init_split.length; i++) // Include "data" text from the data stream 
				 { 
				    dlt.add(i,init_split[i]); 					  // Store the data values in an array list
				 }
				 /*for (int i = 0; i < init_split.length; i++) // Checking the splitting of data stream 
					 System.out.println(init_split[i]);*/
				 
				 Iterator<String> it = dlt.iterator(); 
				 
				/* while (it.hasNext()) 
				 {
				 System.out.println(it.next());                    // Contents of array list 
				 }*/
				 
				 			
				Mongo mongo = new Mongo("localhost", 27017);
				DB db = mongo.getDB("datastore");
				DBCollection collection = db.getCollection("sensordata");
																			//// -todays first edit for json format
				////BasicDBObject document = new BasicDBObject();
				
				DBObject dbObject = null ;
				
				for (int i = 0; i < dlt.size(); i++) 
				 { ++ count;
				 
				    /// dbObject.put("Thing_IP", addrs);                   // -try to append ip of thing
				 
					////document.put("Thing_IP", addrs);  // Insert data into MongoDB
					
					dbObject = (DBObject) JSON.parse("{"+dlt.get(i)+"}");
					
					////document.append("data", dlt.get(i));
					
					System.out.println("Number Of Data Records Received As Of "+ts+" : "+count);
				 }
				 				 
				collection.insert(dbObject);

			}

			
			 
			/* List<String> dlt = new ArrayList<String>(); // Array list to store values
			 
			 try { 
				 
				 for (int i = 1; i <= init_split.length - 1; i++) // Exclude "data" text from the data stream 
					 { 
					    dlt.add(i,init_split[i]);                // Store the data values in an array list
					 }
			 		 
			 
			 for (int i = 0; i < init_split.length; i++)        // Checking the splitting of data stream 
				 System.out.println(init_split[i]);
			 
			 Iterator<String> it = dlt.iterator(); 
			 
			 while (it.hasNext()) 
			 {
			 System.out.println(it.next());                    // Contents of array list 
			 }
			 
			 // To connect to mongodb server MongoClient mongoClient = new
			 MongoClient("localhost", 27017);
			 
			 // Now connect to your databases 
			 DB db = MongoClient.getDB("datastore");
			 System.out.println("Connect to database successfully");
			 DBCollection coll =db.getCollection("sensordata"); 
			 System.out.println("Collection sensordata selected successfully"); 
			 BasicDBObject document = new BasicDBObject(); 
			 document.put("database","thing"); 
			 document.put("table", "sensordata");
			 
			 BasicDBObject documentDetail = new BasicDBObject();
			 
			 documentDetail.put("TokenID", dlt.get(1)); 
			 
			 for (int i = 2; i <=dlt.size() - 1; i++) 
			 { 
				 // Insert data into MongoDB
			 }
			 
			 documentDetail.put("Measure" + i, dlt.get(i));
			 
			 }
			 
			 document.put("detail", documentDetail);
			 
			 WriteResult res = coll.insert(document); 
			
			 */

			catch (Exception e) {
				e.getMessage();
			}

			

		}

	}
}
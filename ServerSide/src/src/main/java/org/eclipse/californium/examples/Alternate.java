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
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.CoapServer;
import org.eclipse.californium.core.network.CoapEndpoint;
import org.eclipse.californium.core.network.config.NetworkConfig;
import org.eclipse.californium.core.server.resources.CoapExchange;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.Mongo;
import com.mongodb.MongoClient;
import com.mongodb.WriteResult;

public class Alternate extends CoapServer {

	private static final int COAP_PORT = NetworkConfig.getStandard().getInt(
			NetworkConfig.Keys.COAP_PORT);

	Vector<String> v = new Vector<String>();
	String str = "NULL";

	/*
	 * Application entry point.
	 */
	public static void main(String[] args) {

		try {

			// create server
			Alternate server = new Alternate();
			// add endpoints on all IP addresses
			server.addEndpoints();
			server.start();

		} catch (SocketException e) {
			System.err
					.println("Failed to initialize server: " + e.getMessage());
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
		for (NetworkInterface ni : Collections.list(NetworkInterface
				.getNetworkInterfaces())) {
			for (InetAddress addr : Collections.list(ni.getInetAddresses())) {
				if (!addr.isLoopbackAddress() && addr instanceof Inet4Address) {
					InetSocketAddress bindToAddress = new InetSocketAddress(
							addr, COAP_PORT);
					addEndpoint(new CoapEndpoint(bindToAddress));
				}
			}
		}
	}

	/*
	 * Constructor for a new Hello-World server. Here, the resources of the
	 * server are initialized.
	 */
	public Alternate() throws SocketException {

		// provide an instance of a Hello-World resource
		add(new HelloWorldResource());
	}

	/*
	 * Definition of the Hello-World Resource
	 */
	class HelloWorldResource extends CoapResource {

		public HelloWorldResource() {

			// set resource identifier
			super("Nikesh");

			// set display name
			getAttributes().setTitle("Hello-World Resource");
		}

		@Override
		public void handleGET(CoapExchange exchange) {

			// respond to the request
			exchange.respond("Hello world for team b iot");
		}

		public void handlePOST(CoapExchange exchange) {

			while (exchange.getRequestText() != "NULL") {
				exchange.accept();
				int i = 1;
				// System.out.println(exchange.getRequestText());

				String reg_tg = exchange.getRequestText();

				v.add(i, reg_tg);
				i++;
			}

			Iterator<String> it = v.iterator();
			while (it.hasNext()) {

				Object ob = it.next();
				String str = ob.toString();
				System.out.println(str);        // check the string in vector
				String init_split[] = str.split(";");
				for (int i = 0; i < init_split.length; i++)
					System.out.println(init_split[i]); // checking the split
														// action enable or
														// disable
				String helchk = "Servercheck";
				if (init_split[0].equalsIgnoreCase(helchk)) { // Health Check
					exchange.respond("Server Alive");
				}

				else if (init_split[0].equalsIgnoreCase("registration") // Registration
																		// Request
						&& init_split[1].equalsIgnoreCase("Rasberry")
						&& init_split[2].equalsIgnoreCase("77745")) {
					// System.out.println(exchange.getRequestText());
					exchange.respond(" Success 3984"); // validating the request
														// action

				}

				// exchange.accept();
				// ////////////String data_tg = exchange.getRequestText(); //
				// Data storage request
				// ////////////String data_split[] = data_tg.split(";");
				// ////////////if(data_split[0].equalsIgnoreCase("data"))
				// ///////////{
				// ///////////
				// ///////////}

				else {
					try {

						// ///////////for(int i=0; i< data_split.length;i++){
						// /////////// System.out.println(data_split[i]);
						// ///////////}
						exchange.respond("Data Inserted Successfully");
						Mongo mongo = new Mongo("localhost", 27017);
						DB db = mongo.getDB("datastore");
						DBCollection collection = db
								.getCollection("sensordata");

						BasicDBObject document = new BasicDBObject();
						document.put("_id", init_split[1]);
						document.append("StreamType", init_split[0]);
						document.append("No of fields", init_split.length);
						/*BasicDBObject details = new BasicDBObject()
								.append("1", init_split[2])
								.append("2", init_split[3])
								.append("3", init_split[4])
								.append("4", init_split[5]);
						ArrayList<BasicDBObject> details_list = new ArrayList<BasicDBObject>();
						details_list.add(details);
						document.append("details", details);
*/
						collection.insert(document);
					}

					/*
					 * 
					 * List<String> dlt = new ArrayList<String>(); // Array list
					 * to store // values
					 * 
					 * try { for (int i = 1; i <= data_split.length - 1; i++) //
					 * Exclude // "data" // text from // the data // stream {
					 * dlt.add(i, data_split[i]); // Store the data values in an
					 * // array list
					 * 
					 * }
					 * 
					 * for (int i = 0; i < data_split.length; i++) // Checking
					 * the splitting of data stream
					 * System.out.println(data_split[i]);
					 * 
					 * Iterator it = dlt.iterator(); while (it.hasNext()) {
					 * System.out.println(it.next()); // Contents of array list
					 * }
					 * 
					 * // To connect to mongodb server MongoClient mongoClient =
					 * new MongoClient("localhost", 27017);
					 * 
					 * // Now connect to your databases DB db =
					 * mongoClient.getDB("datastore");
					 * System.out.println("Connect to database successfully");
					 * DBCollection coll = db.getCollection("sensordata");
					 * System
					 * .out.println("Collection sensordata selected successfully"
					 * ); BasicDBObject document = new BasicDBObject();
					 * document.put("database", "thing"); document.put("table",
					 * "sensordata");
					 * 
					 * BasicDBObject documentDetail = new BasicDBObject();
					 * 
					 * documentDetail.put("TokenID", dlt.get(1)); for (int i =
					 * 2; i <= dlt.size() - 1; i++) { // Insert data into
					 * MongoDB
					 * 
					 * documentDetail.put("Measure" + i, dlt.get(i));
					 * 
					 * }
					 * 
					 * document.put("detail", documentDetail);
					 * 
					 * WriteResult res = coll.insert(document); String str1 =
					 * res.toString(); // Record insertion status
					 * exchange.respond(str1);
					 * 
					 * System.out.println("Document inserted successfully"); /*
					 * Mongo mongo = new Mongo("localhost", 27017); DB db =
					 * mongo.getDB("thing"); DBCollection collection =
					 * db.getCollection("sensordata");
					 * 
					 * BasicDBObject document = new BasicDBObject();
					 * document.put("database", "thing"); document.put("table",
					 * "sensordata");
					 * 
					 * BasicDBObject documentDetail = new BasicDBObject();
					 * 
					 * documentDetail.put("TokenID", dlt.get(1)); for(int
					 * i=2;i<=dlt.size()-1;i++) { // Insert data into MongoDB
					 * 
					 * documentDetail.put("Measure"+i,dlt.get(i));
					 * 
					 * }
					 * 
					 * document.put("detail", documentDetail);
					 * 
					 * WriteResult res =collection.insert(document); String
					 * str1=res.toString(); // Record insertion status
					 * exchange.respond(str1);
					 */

					catch (Exception e) {
						e.getMessage();
					}

					/*
					 * ResponseCode response; synchronized (this) { // critical
					 * section response = ResponseCode.CREATED; }
					 */

				}
			}
		}
	}
}
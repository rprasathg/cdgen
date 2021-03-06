/*******************************************************************************
 *   Copyright (c) 2019 Dortmund University of Applied Sciences and Arts and others.
 *
 *   This program and the accompanying materials are made
 *   available under the terms of the Eclipse Public License 2.0
 *   which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 *   SPDX-License-Identifier: EPL-2.0
 *
 *   Contributors:
 *       Dortmund University of Applied Sciences and Arts - initial API and implementation
 *******************************************************************************/
package org.eclipse.app4mc.cdgen.test;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.List;

import org.eclipse.app4mc.amalthea.model.Amalthea;
import org.eclipse.app4mc.amalthea.model.Task;

/**
 * Implementation of testing Task, Runnable structure and Label types.
 *
 */

public class testMain

{

	public testMain(final Amalthea model, final String path1, final int configFlag, final int k,
			final List<Task> taskmod) throws IOException {
		if ((0x1000 == (configFlag & 0xF000))) {
			fileTestmainTask(model, path1, k, taskmod);
		}
		else if (0x2000 == (configFlag & 0xF000)) {
			fileTestmainTaskPthread(model, path1, k, taskmod);
		}
	}

	private void fileTestmainTaskPthread(final Amalthea model2, final String path1, final int k,
			final List<Task> taskmod) throws IOException {
		final String fname = path1 + File.separator + "main" + k + ".c";
		// EList<Task> taskmod = model2.getSwModel().getTasks();
		final File f1 = new File(fname);
		String[] words = null;
		@SuppressWarnings("resource")
		final FileReader fr = new FileReader(f1);
		@SuppressWarnings("resource")
		final BufferedReader br = new BufferedReader(fr);
		String s;
		final String input = "\t\tpthread_create";
		int count = 0;
		while ((s = br.readLine()) != null) {
			words = s.split(" ");
			for (final String word : words) {
				if (word.equals(input)) {
					count++;
				}
			}
		}
		if (count != taskmod.size()) {
			System.out.println(taskmod.size() + "\tMain" + k + " : ERROR: Task create count\t" + count);
		}
		else {
			System.out.println("Main" + k + " : Task create count OK");
		}
		fr.close();
	}

	private void fileTestmainTask(final Amalthea model2, final String path1, final int k, final List<Task> taskmod)
			throws IOException {
		final String fname = path1 + File.separator + "main" + k + ".c";
		// EList<Task> taskmod = model2.getSwModel().getTasks();
		final File f1 = new File(fname);
		String[] words = null;
		@SuppressWarnings("resource")
		final FileReader fr = new FileReader(f1);
		@SuppressWarnings("resource")
		final BufferedReader br = new BufferedReader(fr);
		String s;
		final String input = "\txTaskCreate(";
		final String input2 = "configMINIMAL_STACK_SIZE,";
		int count = 0, count1 = 0;
		while ((s = br.readLine()) != null) {
			words = s.split(" ");
			for (final String word : words) {
				if (word.equals(input)) {
					count++;
				}
				else if (word.equals(input2)) {
					count1++;
				}
			}
		}
		if (count != taskmod.size()) {
			System.out.println(taskmod.size() + "\tMain" + k + " : ERROR: Task create count\t" + count);
		}
		else {
			System.out.println("Main" + k + " : Task create count OK");
		}
		if (count1 != taskmod.size()) {
			System.out.println(taskmod.size() + "\tMain" + k + " : ERROR: Task Priorities count\t" + count1);
		}
		else {
			System.out.println("Main" + k + " : Task Priorities count OK");
		}
		fr.close();
	}
}


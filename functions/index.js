const functions = require("firebase-functions");
const admin = require("firebase-admin");
const axios = require("axios");

admin.initializeApp();
const db = admin.firestore();

const updateRequest = async (requestRef, payload) => {
  await requestRef.update(payload);
};

const processDeliveryRequest = async (snap) => {
  const req = snap.data() || {};
  const orderId = String(req.orderId || "").trim();

  if (!/^\d{6}$/.test(orderId)) {
    await updateRequest(snap.ref, {
      status: "rejected",
      result: "invalid_code_format",
      message: "orderId must be 6 digits",
      completedAt: admin.firestore.FieldValue.serverTimestamp(),
    });
    return;
  }

  await updateRequest(snap.ref, {
    status: "processing",
    processingAt: admin.firestore.FieldValue.serverTimestamp(),
  });

  const orderQuery = db.collection("orders")
      .where("orderId", "==", orderId)
      .limit(1);
  const orderSnap = await orderQuery.get();

  if (orderSnap.empty) {
    await updateRequest(snap.ref, {
      status: "rejected",
      result: "not_found",
      message: "Code not found",
      completedAt: admin.firestore.FieldValue.serverTimestamp(),
    });
    return;
  }

  const orderDoc = orderSnap.docs[0];
  const order = orderDoc.data();

  if (order.status === "delivered") {
    await updateRequest(snap.ref, {
      status: "rejected",
      result: "already_delivered",
      message: "Order already delivered",
      completedAt: admin.firestore.FieldValue.serverTimestamp(),
    });
    return;
  }

  await orderDoc.ref.update({
    status: "delivered",
    deliveredAt: admin.firestore.FieldValue.serverTimestamp(),
  });

  let lockResult = "skipped";
  let notifyResult = "skipped";
  let notifyError = "";
  const cfg = functions.config();
  const lockerCfg = (cfg && cfg.locker) ? cfg.locker : {};

  try {
    const lockEndpoint = lockerCfg.lock_endpoint || "";
    if (lockEndpoint) {
      await axios.post(
          lockEndpoint,
          {orderId: orderId, action: "open_lock"},
          {timeout: 5000},
      );
      lockResult = "opened";
    }
  } catch (e) {
    lockResult = "failed";
  }

  try {
    const topic = lockerCfg.ntfy_topic || "";
    if (topic) {
      const title = encodeURIComponent("Package Delivered");
      const url = [
        "https://ntfy.sh/" + encodeURIComponent(topic),
        "?title=" + title,
        "&tags=package,white_check_mark",
        "&priority=default",
      ].join("");
      const message = [
        "Hi " + (order.customerName || "customer") + ", your item",
        "\"" + (order.itemName || "") + "\" has arrived.",
        "Pickup ID: " + orderId + ".",
      ].join(" ");
      await axios.post(url, message, {
        headers: {"Content-Type": "text/plain; charset=utf-8"},
        timeout: 5000,
      });
      notifyResult = "sent";
    }
  } catch (e) {
    notifyResult = "failed";
    notifyError = e.message || "ntfy error";
  }

  await updateRequest(snap.ref, {
    status: "confirmed",
    result: "delivered",
    lockResult: lockResult,
    notifyResult: notifyResult,
    notifyError: notifyError,
    completedAt: admin.firestore.FieldValue.serverTimestamp(),
  });
};

exports.processDeliveryRequest = functions.firestore
    .document("delivery_requests/{requestId}")
    .onCreate(processDeliveryRequest);

from flask import Flask, request, jsonify, render_template, redirect, url_for, send_from_directory
from flask_httpauth import HTTPBasicAuth
from werkzeug.security import generate_password_hash, check_password_hash
from werkzeug.utils import secure_filename
import os 
import time 
import logging 
from datetime import datetime, timedelta
import json
from collections import deque, Counter

# store last 100 readings 
humidity_history = deque(maxlen=100)

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

# Create flask app 
app = Flask(__name__, template_folder='../../templates', static_folder='../../static')
auth = HTTPBasicAuth()

users = {
    os.environ.get('AUTH_USERNAME'): generate_password_hash(os.environ.get('AUTH_PASSWORD'))
}

if None in users or not users:
    raise ValueError("AUTH_USERNAME and AUTH_PASSWORD environment variables must be set")

# Store the current alert status
current_alert = {
    "status": "good",
    "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
}

def load_history_from_file():
    """Load history from file on startup"""
    try:
        with open('humidity_history.json', 'r') as f:
            data = json.load(f)
            for item in data:
                humidity_history.append(item)
        logger.info(f"Loaded {len(data)} historical records")
    except FileNotFoundError:
        logger.info("No historical data file found")
    except Exception as e:
        logger.error(f"Error loading history: {str(e)}")

def save_history_to_file():
    """Save history to file"""
    try:
        with open('humidity_history.json', 'w') as f:
            json.dump(list(humidity_history), f)
        logger.debug("History saved to file")
    except Exception as e:
        logger.error(f"Error saving history: {str(e)}")

def get_status_analytics():
    """Calculate analytics based on status history"""
    if not humidity_history:
        return {
            'total_readings': 0,
            'status_distribution': {},
            'recent_24h': {
                'total': 0,
                'good': 0,
                'warning_approaching': 0,
                'warning_bad': 0,
                'alert_percentage': 0
            },
            'status_timeline': [],
            'alert_frequency': {},
            'uptime_percentage': 0,
            'longest_good_streak': 0,
            'current_streak': {'status': 'unknown', 'count': 0}
        }
    
    # Get readings from last 24 hours
    now = datetime.now()
    recent_readings = []
    
    for reading in humidity_history:
        try:
            reading_time = datetime.strptime(reading['timestamp'], "%Y-%m-%d %H:%M:%S")
            if reading_time > now - timedelta(days=1):
                recent_readings.append(reading)
        except (ValueError, KeyError):
            continue
    
    # Overall status distribution
    all_statuses = [r['status'] for r in humidity_history]
    status_counts = Counter(all_statuses)
    total_readings = len(humidity_history)
    
    status_distribution = {
        'good': status_counts.get('good', 0),
        'warning_approaching': status_counts.get('warning(approaching bad)', 0),
        'warning_bad': status_counts.get('warning(bad)', 0)
    }
    
    # 24-hour analysis
    recent_statuses = [r['status'] for r in recent_readings]
    recent_counts = Counter(recent_statuses)
    recent_total = len(recent_readings)
    
    recent_24h = {
        'total': recent_total,
        'good': recent_counts.get('good', 0),
        'warning_approaching': recent_counts.get('warning(approaching bad)', 0),
        'warning_bad': recent_counts.get('warning(bad)', 0),
        'alert_percentage': 0
    }
    
    if recent_total > 0:
        alert_count = recent_24h['warning_approaching'] + recent_24h['warning_bad']
        recent_24h['alert_percentage'] = round((alert_count / recent_total) * 100, 1)
    
    # Status timeline for charting (last 50 readings)
    timeline_data = []
    for reading in list(humidity_history)[-50:]:
        # Convert status to numeric for charting
        status_value = {
            'good': 3,
            'warning(approaching bad)': 2,
            'warning(bad)': 1
        }.get(reading['status'], 0)
        
        timeline_data.append({
            'timestamp': reading['timestamp'],
            'status': reading['status'],
            'value': status_value,
            'time_only': reading['timestamp'][-8:-3]  # Extract HH:MM
        })
    
    # Alert frequency by hour
    alert_frequency = {}
    for reading in humidity_history:
        if reading['status'] != 'good':
            try:
                hour = datetime.strptime(reading['timestamp'], "%Y-%m-%d %H:%M:%S").hour
                alert_frequency[hour] = alert_frequency.get(hour, 0) + 1
            except ValueError:
                continue
    
    # Uptime percentage (good status)
    uptime_percentage = round((status_distribution['good'] / total_readings) * 100, 1) if total_readings > 0 else 0
    
    # Streak analysis
    longest_good_streak = 0
    current_good_streak = 0
    current_streak = {'status': 'unknown', 'count': 0}
    
    if humidity_history:
        # Calculate streaks
        for i, reading in enumerate(humidity_history):
            if reading['status'] == 'good':
                current_good_streak += 1
                longest_good_streak = max(longest_good_streak, current_good_streak)
            else:
                current_good_streak = 0
        
        # Current streak
        last_status = humidity_history[-1]['status']
        count = 1
        for reading in reversed(list(humidity_history)[:-1]):
            if reading['status'] == last_status:
                count += 1
            else:
                break
        
        current_streak = {
            'status': last_status,
            'count': count
        }
    
    return {
        'total_readings': total_readings,
        'status_distribution': status_distribution,
        'recent_24h': recent_24h,
        'status_timeline': timeline_data,
        'alert_frequency': alert_frequency,
        'uptime_percentage': uptime_percentage,
        'longest_good_streak': longest_good_streak,
        'current_streak': current_streak
    }

@auth.verify_password
def verify_password(username, password):
    if username in users and check_password_hash(users.get(username), password):
        return username 
    return None

@app.route('/ping')
def ping():
    return "Server is running"

# Home 
@app.route('/')
@auth.login_required
def index():
    logger.debug("Accessing index page")
    try:
        logger.debug(f"Current alert status: {current_alert['status']}")
        
        # Get analytics
        analytics = get_status_analytics()
        
        return render_template('index.html', 
                             alert=current_alert, 
                             history=list(humidity_history),
                             analytics=analytics)
    except Exception as e: 
        logger.error(f"Error in index route: {str(e)}")
        return f"Error: {str(e)}", 500

# Alert update endpoint - keeping your original design
@app.route('/update-alert', methods=['POST'])
def update_alert():
    logger.debug("Alert update endpoint hit")
    
    try:
        # Get data from request
        data = request.get_json()
        
        if not data or 'status' not in data:
            logger.error("No alert status in request")
            return jsonify({'error': 'Missing alert status'}), 400
        
        # Validate the alert status
        status = data['status']
        valid_statuses = ["good", "warning(approaching bad)", "warning(bad)"]
        
        if status not in valid_statuses:
            logger.error(f"Invalid alert status: {status}")
            return jsonify({'error': 'Invalid alert status'}), 400
        
        # Update the current alert with timestamp generated here
        current_alert["status"] = status
        current_alert["timestamp"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        logger.info(f"Alert updated to: {status}")
        
        # Add to history (create a copy) - FIXED: was current_alert.json()
        humidity_history.append(current_alert.copy())
        
        # Save to file periodically (every 10 readings)
        if len(humidity_history) % 10 == 0:
            save_history_to_file()
        
        return jsonify({
            'message': 'Alert updated successfully',
            'status': status,
            'timestamp': current_alert["timestamp"]
        }), 200
        
    except Exception as e:
        logger.error(f"Error updating alert: {str(e)}")
        return jsonify({'error': str(e)}), 500

# Data export endpoint
@app.route('/export-data')
@auth.login_required
def export_data():
    """Export status data as CSV"""
    try:
        # Create CSV content
        csv_lines = ["Timestamp,Status"]
        for reading in humidity_history:
            timestamp = reading.get('timestamp', 'N/A')
            status = reading.get('status', 'N/A')
            csv_lines.append(f"{timestamp},{status}")
        
        # Create response
        csv_content = '\n'.join(csv_lines)
        response = app.make_response(csv_content)
        response.headers['Content-Type'] = 'text/csv'
        response.headers['Content-Disposition'] = f'attachment; filename=humidity_status_data_{datetime.now().strftime("%Y%m%d_%H%M%S")}.csv'
        
        return response
    except Exception as e:
        logger.error(f"Error exporting data: {str(e)}")
        return f"Error exporting data: {str(e)}", 500

# Load existing history on startup
load_history_from_file()

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 8080))
    logger.info(f"Starting server on port {port}")
    
    # Save history on shutdown
    import atexit
    atexit.register(save_history_to_file)
    
    app.run(host="0.0.0.0", port=port)